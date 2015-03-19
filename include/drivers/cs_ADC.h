/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <stdint.h>

//#include "common/cs_CircularBuffer.h"
#include "common/cs_DifferentialBuffer.h"
#include "events/cs_Dispatcher.h"
#include "cs_RTC.h"

/*
 * Analog digital conversion class. 
 */

//#define ADC_BUFFER_SIZE 131
#define ADC_BUFFER_SIZE 175
#define DEFAULT_RECORDING_THRESHOLD 1


/*
 * Class to store the samples
 *
 * The samples are compressed by storing only the difference with the first previous sample.
 */
class AdcSamples {
public:
	AdcSamples(): _writeLock(false), _voltageBuf(NULL), _timestampBuf(NULL) {

	}

	~AdcSamples() {
		delete _voltageBuf;
		delete _timestampBuf;
	}

	/* Initializes the buffer with a certain capacity
	 *
	 */
	bool init(uint16_t capacity) {
		if (_voltageBuf == NULL || _voltageBuf->capacity() != capacity) {
			delete _voltageBuf;
			_voltageBuf = new DifferentialBuffer<uint16_t>(capacity);
		}
		if (!_voltageBuf->init()) {
			log(FATAL, "Could not initialize buffer. Too big!?");
			return false;
		}
		if (_timestampBuf == NULL || _timestampBuf->capacity() != capacity) {
			delete _timestampBuf;
			_timestampBuf = new DifferentialBuffer<uint32_t>(capacity);
		}
		if (!_timestampBuf->init()) {
			log(FATAL, "Could not initialize buffer. Too big!?");
			return false;
		}

		return true;
	}

	void lock() {
		_writeLock = true;
	}

	void unlock() {
		_writeLock = false;
	}

	bool isLocked() {
		return _writeLock;
	}

	uint16_t getSerializedLength() {
//		return _voltageBuf->getSerializedLength() + _timestampBuf->getSerializedLength();
		return sizeof(uint16_t) + _voltageBuf->getSerializedLength() + 2*sizeof(uint32_t);
	}

	void serialize(uint8_t* buf) {
		uint8_t* ptr = buf;
		if (!size()) {
			// First two bytes are the amount of elements
			*ptr++ = 0;
			*ptr++ = 0;
			return;
		}
//		_voltageBuf->serialize(buf);
//		_timestampBuf->serialize(buf+_voltageBuf->getSerializedLength());


		// Only add the first and last timestamp
		// Usually each time step is 3 to 4, so check if there is no big gap

		// Rough check, should actually check each difference
		uint32_t tStart=0;
		uint32_t tEnd=0;
		_timestampBuf->getFirstElement(tStart);
		_timestampBuf->getLastElement(tEnd);
		if (tEnd-tStart > size()*4) {
			LOGd("timestamps are too far apart");
			// First two bytes are the amount of elements
			*ptr++ = 0;
			*ptr++ = 0;
			return;
		}

		// First two bytes are the amount of elements
		*ptr++ = (size() >> 8) & 0xFF;
		*ptr++ = size() & 0xFF;

		_voltageBuf->serialize(ptr);

		ptr = buf+_voltageBuf->getSerializedLength();
		*ptr++ = (tStart >> 24) & 0xFF;
		*ptr++ = (tStart >> 16) & 0xFF;
		*ptr++ = (tStart >> 8) & 0xFF;
		*ptr++ = tStart & 0xFF;
		*ptr++ = (tEnd >> 24) & 0xFF;
		*ptr++ = (tEnd >> 16) & 0xFF;
		*ptr++ = (tEnd >> 8) & 0xFF;
		*ptr++ = tEnd & 0xFF;
	}

	bool getFirstElement(uint32_t& timestamp, uint16_t& voltage) {
		bool res = true;
		res &= _voltageBuf->getFirstElement(voltage);
		res &= _timestampBuf->getFirstElement(timestamp);
		return res;
	}

	bool getNextElement(uint32_t& timestamp, uint16_t& voltage) {
		bool res = true;
		res &= _voltageBuf->getNextElement(voltage);
		res &= _timestampBuf->getNextElement(timestamp);
		return res;
	}


	uint16_t size() {
		return _voltageBuf->size();
	}

	uint16_t full() {
		return _voltageBuf->full();
	}

	void push(uint16_t voltage) {
		_voltageBuf->push(voltage);
	}

	void push(uint16_t voltage, uint32_t timestamp) {
		bool res = true;
		res &= _voltageBuf->push(voltage);
		res &= _timestampBuf->push(timestamp);
		if (!res) {
			_voltageBuf->clear();
			_timestampBuf->clear();
		}
	}

private:
	bool _writeLock;
	DifferentialBuffer<uint16_t>* _voltageBuf;
	DifferentialBuffer<uint32_t>* _timestampBuf;
};


//class AdcSamples {
//public:
//	AdcSamples(): _writeLock(false), _size(0), _increments(NULL) {
//
//	}
//
//	~AdcSamples() {
//		delete _increments;
//	}
//
//	/* Initializes the buffer with a certain capacity
//	 *
//	 */
//	bool init(uint16_t capacity) {
//		if (_increments == NULL || _increments->capacity() != capacity-1) {
//			delete _increments;
//			_increments = new CircularBuffer<int8_t>(capacity-1);
//		}
//		if (!_increments->init()) {
//			log(FATAL, "Could not initialize buffer. Too big!?");
//			return false;
//		}
//		clear();
//		return true;
//	}
//
//	/* Clears the buffer
//	 *
//	 */
//	void clear() {
//		_increments->clear();
//		_size=0;
//		_timeStart=0;
//		_timeEnd=0;
//	}
//
//	/* Write samples into a buffer.
//	 *
//	 * Make sure the buffer is large enough to contain size() elements.
//	 * Locks the buffer
//	 */
//	void getSamples(uint16_t* buf) {
//		_writeLock = true;
//		if (!_size) {
//			_writeLock = false;
//			return;
//		}
//		buf[0] = _firstSample;
//		for (uint16_t i=0; i<_increments->size(); ++i) {
//			buf[i+1] = buf[i]+_increments->operator[](i);
//		}
//		_writeLock = false;
//	}
//
//	/* Returns the first sample in the buffer
//	 *
//	 * Locks the buffer.
//	 * If the buffer is empty, it returns (uint16_t)-1 and unlocks
//	 */
//	uint16_t getFirstSample() {
//		_writeLock = true;
//		if (!_size) {
//			_writeLock = false;
//			return (uint16_t)-1;
//		}
//		_readIdx = 0;
//		_readSample = _firstSample;
////		LOGd("f=%u s=%u", _firstSample, _size);
//		return _readSample;
//	}
//
//	/* Returns the next sample in the buffer
//	 *
//	 * Make sure you first called getFirstSample().
//	 * When the end has been reached: unlocks and returns (uint16_t)-1.
//	 */
//	uint16_t getNextSample() {
//		if (!_writeLock) {
//			return (uint16_t)-1;
//		}
////		LOGd("idx=%u inc=%i", _readIdx, _increments->operator [](_readIdx));
//		if (_readIdx >= _increments->size()) {
//			_writeLock = false;
//			return (uint16_t)-1;
//		}
//		_readSample += _increments->operator [](_readIdx++);
//		return _readSample;
//	}
//
//	/* Returns the number of elements in the buffer
//	 *
//	 */
//	inline uint16_t size() {
//		return _size;
//	}
//
//	inline uint16_t capacity() {
//		return _increments->capacity()+1;
//	}
//
//	inline bool empty() {
//		return _size == 0;
//	}
//
//	inline bool full() {
//		return _increments->full();
//	}
//
//
//	/* Pushes a value to the buffer
//	 *
//	 */
//	void push(uint16_t val) {
//		if (!_size) {
//			_firstSample = val;
//			_lastSample = val;
//			_size=1;
////			LOGd("val=%u f=%u l=%u s=%u", val, _firstSample, _lastSample, _size);
//			return;
//		}
//
//		int16_t diff = (int16_t)val - _lastSample;
//		if (diff > 127 || diff < -127) {
//			LOGe("difference too large! %i", diff);
////			LOGe("previous val=%u, current val=%u", _lastSample, val);
//			clear();
//			return;
//		}
//
//		if (_increments->full()) {
//			_firstSample += _increments->peek();
//			--_size;
//		}
//
//		_increments->push(diff);
//		_lastSample = val;
//		++_size;
////		LOGd("val=%u f=%u l=%u s=%u d=%i", val, _firstSample, _lastSample, _size, diff);
//	}
//
//	/* Returns whether or not the buffer is locked for writing by ADC
//	 *
//	 */
//	bool isLocked() {
//		return _writeLock;
//	}
//
//	uint32_t _timeStart;
//	uint32_t _timeEnd;
//
//private:
//	bool _writeLock;
//	uint16_t _readIdx;
//	uint16_t _readSample;
//
//	uint16_t _size;
//	uint16_t _firstSample;
//	uint16_t _lastSample;
//
//	CircularBuffer<int8_t>* _increments;
//};

class ADC: public Dispatcher {

public:
	// Use static variant of singleton, no dynamic memory allocation
	static ADC& getInstance() {
		static ADC instance;
		return instance;
	}

	/* If decorated with a real time clock, we can "timestamp" the adc values.
	 *
	 * @clock Clock that is running.
	 */
	void setClock(RealTimeClock &clock);

	/* Initialize ADC, this will allocate memory for the samples.
	 *
	 * @pin The pin to use as input (AIN<pin>).
	 */
	uint32_t init(uint8_t pin);

	// Start the ADC sampling
	void start();

	// Stop the ADC sampling
	void stop();

	// Each tick we have time to dispatch events e.g.
	void tick();

	/* Return reference to the sampled data
	 *
	 * @return a reference to the sampled data
	 */
	AdcSamples* getSamples();

	// return reference to internal buffer
//	CircularBuffer<uint16_t>* getBuffer();

	/* Set threshold to start writing samples to buffer.
	 *
	 * Default threshold is DEFAULT_RECORDING_THRESHOLD
	 */
	inline void setThreshold(uint8_t threshold) { _threshold = threshold; }

	// Function to be called from interrupt, do not do much there!
	void update(uint16_t value);

private:
	/* Constructor
	 * Will not allocate memory yet.
	 */
	ADC(): _bufferSize(ADC_BUFFER_SIZE),
		_threshold(DEFAULT_RECORDING_THRESHOLD),
       		_clock(NULL) {}

	// This class is singleton, deny implementation
	ADC(ADC const&);
	// This class is singleton, deny implementation
	void operator=(ADC const &);

	uint16_t _bufferSize;
	uint16_t _lastResult;
	uint16_t _numSamples;
	bool _store;
	uint8_t _threshold;

	RealTimeClock *_clock;

	uint32_t config(uint8_t pin);
};
