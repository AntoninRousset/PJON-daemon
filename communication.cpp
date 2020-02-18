/* 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* Maximum accepted timeframe between transmission and synchronous
   acknowledgement. This timeframe is affected by latency and CRC computation.
   Could be necessary to higher this value if devices are separated by long
   physical distance and or if transmitting long packets. */
#define TSA_RESPONSE_TIME_OUT 100000
#define PJON_INCLUDE_TSA true // Only include ThroughSerialAsync

#include "communication.hpp"
#include "logger.hpp"

#include <map>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include "PJON.h"

#define min(a, b) (a > b ? b : a)
#define max(a, b) (a > b ? a : b)

class Packet {

	public:

		Packet(com_id dest, size_t n, const void* data);
		float period;
		uint32_t registration;
		uint32_t timing;
		uint16_t length;
		uint16_t state;
		com_id dest;
		uint8_t  attempts;
		char     content[PJON_PACKET_MAX_LENGTH];

};

template<typename T>
class ApproxFloatingMean {

	public:

		ApproxFloatingMean(unsigned int n, T initial_value=0)
		{
			this->n = n;
			this->mean = initial_value;
		}

		T push(T v)
		{
			this->mean = this->mean + (v - this->mean) / this->n;
			return this->get();
		}

		T get()
		{
			return this->mean;
		}

	private:

		unsigned int n;
		T mean;
};

static PJON<ThroughSerialAsync> bus;
static std::map<com_ref, Packet> packets;
static unsigned int max_attempts = 32;
static uint32_t baudrate;
static char* serial_device_path = nullptr;
static float initial_period = 10; // in us
static float period_factor = 1.2;
static com_message reception[COM_MAX_INCOMING_MESSAGES];
static unsigned int reception_p = 0;
static bool state_log_connected = true;
static ApproxFloatingMean<float> success_rate(16, 1.0);
static ApproxFloatingMean<float> ping(8, 0);

static void receiver(uint8_t * data, uint16_t n, const PJON_Packet_Info &packet_info);
static void record_ping(float t);
static void record_success_rate(bool success);

Packet::Packet(com_id dest, size_t n, const void* data)
{
	this->registration = PJON_MICROS();
	this->timing = this->registration;
	this->period = initial_period;
	this->length = n;
	this->state = (n > PJON_PACKET_MAX_LENGTH) ? PJON_CONTENT_TOO_LONG : PJON_TO_BE_SENT;
	this->dest = dest;
	this->attempts = 0;
	if (this->state != PJON_CONTENT_TOO_LONG)
		memcpy(this->content, data, n); 
}

bool com_init(com_id id, const char *dev, uint32_t bd)
{
	log_info("com", "Initialization with id: 0x%02x", id);
	bus.set_id(id);
	bus.set_receiver(receiver);
	baudrate = bd;
	if (serial_device_path)
		free(serial_device_path);
	serial_device_path = (char*) malloc((strlen(dev)+1)*sizeof(char));
	strcpy(serial_device_path, dev);
	return true;
}

void com_set_max_attempts(unsigned int m)
{
	max_attempts = m;
}

void com_set_time_period(float t0, float f)
{
	initial_period = t0;
	period_factor = f;
}

bool com_connect()
{
	// open serial
	bus.strategy.set_serial(serialOpen(serial_device_path, baudrate));
	if (!com_is_connected()) {
		if (state_log_connected)
			log_error("com", "Failed to open serial device: %s", serial_device_path);
		state_log_connected = false;
		return false;
	}
	state_log_connected = true;
	log_info("com", "Serial device opened: %s", serial_device_path);

	// setting bus
	log_info("com", "Setting up bus with baudrate = %ld", baudrate);
	bus.strategy.set_baud_rate(baudrate);
	bus.set_synchronous_acknowledge(true);
	bus.set_asynchronous_acknowledge(false);
	bus.begin();

	return true;
}

//TODO be sure of the implementation -> seems ok -> more tests?
bool com_is_connected(void)
{
	fd_set nfds;
	FD_ZERO(&nfds);
	FD_SET(bus.strategy.serial, &nfds);

	if (bus.strategy.serial < 0)
		return false;

	struct timeval tv = {0, 1};
	select(bus.strategy.serial+1, &nfds, NULL, NULL, &tv);
	if (FD_ISSET(bus.strategy.serial, &nfds)) {
		size_t len = 0;
		ioctl(bus.strategy.serial, FIONREAD, &len);
		if(len == 0) // no data available -> disconnected
			return false;
	}

	return true;
}

bool com_push(com_ref r, com_id dest, size_t n, const void* data)
{
	packets.insert(std::pair<com_ref, Packet>(r, Packet(dest, n, data)));
	if (packets.size() > COM_OUTGOING_QUEUE_WARNING_THRESHOLD) {
		log_warn("com", "Outgoing packets queue is filling up %d/%d",
				packets.size(), COM_OUTGOING_QUEUE_WARNING_THRESHOLD);
	}
	return true;
}

void com_cancel(com_ref r)
{
	auto it = packets.find(r);
	if (it != packets.end())
		packets.erase(r);
}

size_t com_send(com_request * results, size_t n_max)
{
	size_t n = 0;
	auto to_cancel = std::vector<com_ref>();

	for (auto &it : packets) {

		auto r = it.first;
		auto &p = it.second;

		// break in case of full results array
		if (n >= n_max)
			return n;

		// send only if dt >= period
		if (PJON_MICROS() - p.timing < p.period)
			continue;

		// CONTENT_TOO_LONG
		if (p.state == PJON_CONTENT_TOO_LONG) {
			log_warn("com", "COM_CONTENT_TOO_LONG for request ref=%d", r);
			results[n] = (com_request){r, COM_CONTENT_TOO_LONG};
			n++;
			to_cancel.push_back(r);
			record_success_rate(false);
			continue;
		}

		p.state = bus.send_packet(p.dest, (char*) p.content, p.length);
		p.attempts++;
		p.timing = PJON_MICROS();
		p.period *= period_factor;

		// SUCCESS 
		if (p.state == PJON_ACK) {
			log_info("com", "COM_SUCCESS for request ref=%d after t=%'ldus", r,
					p.timing-p.registration);
			results[n] = (com_request){r, COM_SUCCESS};
			n++;
			record_success_rate(true);
			record_ping(p.timing-p.registration);
			to_cancel.push_back(r);
			continue;
		}

		// too much attempts -> CONNECTION_LOST
		if (p.attempts > max_attempts) {
			log_warn("com", "COM_CONNECTION_LOST for request %d (dest: 0x%02x)", r,
					p.dest);
			results[n] = (com_request){r, COM_CONNECTION_LOST};
			n++;
			to_cancel.push_back(r);
			record_success_rate(false);
			continue;
		}

	}

	for (auto &r : to_cancel)
		com_cancel(r);

	return n;
}

size_t com_receive(com_message *m, size_t n_max)
{
	bus.receive(10000);
	size_t n = min(n_max, reception_p);
	reception_p = 0;
	memcpy(m, reception, n*sizeof(com_message));
	return n;
}

void com_quit()
{
}

void receiver(uint8_t * data, uint16_t n, const PJON_Packet_Info &packet_info)
{
	log_info("com", "Reception: (%d) %.*s / %d\n", n, n, data, reception_p);
	reception[reception_p].src = packet_info.sender_id;
	reception[reception_p].n = n;
	memcpy(&reception[reception_p].data, data,
			sizeof(reception[reception_p].data));
	reception_p++;
}

void record_ping(float t)
{
	if (ping.push(t) >= COM_PING_WARNING_THRESHOLD) {
		log_warn("com", "Ping is high: %.3fms (warning threshold: %.3fms)",
				ping.get()/1000.f, COM_PING_WARNING_THRESHOLD/1000.f);
	}
}

void record_success_rate(bool success)
{
	if (success_rate.push(success) <= COM_SUCCESS_RATE_WARNING_THRESHOLD) {
		log_warn("com", "Success rate is low: %.2f\% (warning threshold: %.2f\%)",
				success_rate.get()*100.f, COM_SUCCESS_RATE_WARNING_THRESHOLD*100.f);
	}
}

