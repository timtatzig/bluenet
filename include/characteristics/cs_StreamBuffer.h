/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "ble_gap.h"
#include "ble_gatts.h"

#include "cs_BluetoothLE.h"
#include "characteristics/cs_Serializable.h"
#include <string>
#include "util/cs_Utils.h"

using namespace BLEpp;

/**
 * General typename That can be used to send arrays of values over Bluetooth, of which the first byte is a type
 * or identifier byte, the second one the length of the payload (so, minus identifier and length byte itself)
 * and the next bytes are the payload itself.
 */
template <typename T, int I>
class StreamBufferGeneric : public Serializable {


public:
	/* Define the size of the header in bytes:
	 *
	 * 1 Byte for the type
	 * x Byte for the length, where x is defined by type uint8_t
	 */
	#define SB_HEADER_SIZE     sizeof(uint8_t) + sizeof(uint8_t)

	/* Default constructor
	 *
	 * Create and initialize a stream buffer. The payload array is NOT initialized
	 * for this constructor. In order to use the stream buffer, <StreamBufferGeneric::init()>
	 * has to be called first. If you know the capacity at creation, you can use
	 * the constructor <StreamBufferGeneric(uint8_t capacity)
	 */
	StreamBufferGeneric() :_type(0), _plength(0), _payload(NULL), _capacity(0) {
	};

	/* Constructor initializes also the payload array
	 *
	 * @capacity the capacity with which the payload array should be initialized
	 *
	 * For this constructor the payload array is initialized with size
	 * capacity * sizeof(T)
	 */
	StreamBufferGeneric(uint8_t capacity) : StreamBufferGeneric() {
		init(capacity);
	}

	/* Initialize the payload array
	 *
	 * @param capacity the capacity with which the payload should be initialized
	 *
	 * If the object was initialized beforehand already, the old payload array
	 * is freed prior to the new allocation.
	 */
	void init(uint8_t capacity) {
		if (capacity > I) {
			LOGe("capacity too big");
			return;
		}
		if (_payload) {
			free(_payload);
		}
		_capacity = capacity;
		_payload = (T*)calloc(capacity, sizeof(T));
		LOGi("done");
	}

	/* @inherit */
	bool operator!=(const StreamBufferGeneric &other) {
		if (_type != other._type) return true;
		if (_plength != other._plength) return true;
		if (memcmp(_payload, other._payload, _capacity * sizeof(T)) != 0) return true;
		return false;
	}

	/* Create a string from payload. 
	 *
	 * This creates a C++ string from the uint8_t payload array. Note that this not always makes sense! It will
	 * use the _plength field to establish the length of the array to be used. So, the array does not have to 
	 * have a null-terminated byte.
	 */
	bool toString(std::string &str) {
		if (!_plength) return false;
		str = std::string((char*)_payload, _plength);
		return true;
	}

	/* Create a StreamBufferGeneric object from the string
	 *
	 * @str the string with which the buffer should be initialized
	 *
	 * The stream buffer is created with a capacity defined by the
	 * size of the string. then the string content is copied to the
	 * payload array
	 *
	 * *Note* needs specialization, for types other than uint8_t
	 *
	 * @return the created StreamBufferGeneric object
	 */
	//	static StreamBufferGeneric<T, I>* fromString(std::string& str);

	/* Create a StreamBufferGeneric object from the payload
	 *
	 * @payload pointer to the payload with which the stream buffer
	 *   should be filled
	 *
	 * @length length (number of bytes) of the array pointed to by
	 *   @payload
	 *
	 * The stream buffer is created with the capacity defined by
	 * @length. then the payload from @payload is copied to the
	 * buffer's payload array
	 *
	 * @return the created StreamBufferGeneric object
	 */
	static StreamBufferGeneric<T, I>* fromPayload(T *payload, uint8_t plength) {
		StreamBufferGeneric<T, I>* buffer = new StreamBufferGeneric<T, I>(plength);
		buffer->_plength = plength;
		memcpy(buffer->_payload, payload, buffer->_plength * sizeof(T));
		return buffer;
	}

	/* Fill payload with string.
	 *
	 * @str the string to be copied to the payload.
	 *
	 * If the length of the string is bigger than the capacity of
	 * the stream buffer, the string will be capped at the size of the
	 * stream buffer.
	 *
	 * @return true if successful, false otherwise
	 */
	bool setString(const std::string & str) {
		if (!_payload) {
			LOGe("buffer not initialized!");
			return false;
		}
		_plength = str.length();
		_plength =
			((_plength > _capacity) ? _capacity : _plength);
		memcpy(_payload, str.c_str(), _plength * sizeof(T));
		return true;
	}

	/* Add a value to the stream buffer
	 *
	 * @value the value to be added
	 *
	 * Adds the value to the buffer if it is initialized and
	 * not full
	 *
	 * @return 0 on SUCCESS,
	 *         1 if buffer is full,
	 *         2 if buffer is not initialized
	 */
	uint8_t add(T value) {
		if (!_payload) {
			LOGe("buffer not initialized!");
			return 2;
		}
		if (_plength >= _capacity) return 1;
		_payload[_plength++] = value;
		return 0;
	}

	/* Clear the buffer
	 *
	 * Reset the payload array and set <_plength> to 0
	 */
	void clear() {
		if (!_payload) {
			LOGe("buffer not initialized!");
			return;
		}
		memset(_payload, 0, _capacity * sizeof(T));
		_plength = 0;
	}

	/* Return the type assigned to the SreamBuffer
	 *
	 * @return the type, see <ConfigurationTypes>
	 */
	inline uint8_t type() const { return _type; } 

	/* Get the length/size of the payload in number
	 * of elements
	 *
	 * @return number of elements stored
	 */
	inline uint8_t length() const { return _plength; }

	/* Get a pointer to the payload array
	 *
	 * @return pointer to the array used to store
	 *   the elements of the stream buffer
	 */
	inline T* payload() const { return _payload; }

	/* Set the type for this stream buffer
	 *
	 * @type the type, see <ConfigurationTypes>
	 */
	inline void setType(uint8_t type) { _type = type; }

	/* Set payload of the buffer.
	 *
	 * @payload pointer to the buffer containing the
	 *   elements which should be copied to this stream
	 *   buffer
	 *
	 * @length number of elements in the payload
	 *
	 * If plength is bigger than the capacity, only the
	 * first capacity elements will be copied
	 */
	void setPayload(T *payload, uint8_t plength) {
		if (!_payload) {
			LOGe("buffer not initialized!");
			return;
		}
		_plength =
			((plength > _capacity) ? _capacity : plength);
		memcpy(_payload, payload, _plength * sizeof(T));
	}

	//////////// serializable ////////////////////////////

	/* @inherit */
    uint16_t getSerializedLength() const {
    	return _plength * sizeof(T) + SB_HEADER_SIZE;
    }

	/* @inherit */
    uint16_t getMaxLength() const {
    	LOGi("getMaxLength, capacity: %d, I: %d", _capacity, I);
    	return I * sizeof(T) + SB_HEADER_SIZE;
    }

	/* Serialize entire class.
	 *
	 * This is used to stream this class over BLE. Note that length here includes the field for type and length,
	 * and is hence larger than plength (which is just the length of the payload).
	 *
	 * Needs specialization
	 */
    void serialize(uint8_t* buffer, uint16_t length) const {
    	LOGi("wtf");
    }

	/* Deserialize class
	 *
	 * Needs specialization
	 */
    void deserialize(uint8_t* buffer, uint16_t length) {
    	LOGi("wtf");
    }

protected:
	/* type enum, defines the type of data in the payload, see <ConfigurationTypes> */
	uint8_t _type;

	/* length of the payload (in number of elements) */
	uint8_t _plength;

	/* pointer to the array storing the payload */
	T* _payload;

	/* capacity, maximum length of the payload. array is
	 * initialized with capacity * sizeof(T) bytes
	 */
	uint8_t _capacity;

};

template<typename T, int I = 32>
class StreamBuffer : public StreamBufferGeneric<T, I> {
public:
};

template<int I>
class StreamBuffer<uint8_t, I> : public StreamBufferGeneric<uint8_t, I> {
public:
	StreamBuffer() : StreamBufferGeneric<uint8_t, I>() {}

	StreamBuffer(uint8_t capacity) : StreamBufferGeneric<uint8_t, I>(capacity) {}

	static StreamBuffer<uint8_t, I>* fromString(std::string& str) {
		StreamBuffer<uint8_t, I>* buffer = new StreamBuffer<uint8_t, I>(str.length());
		buffer->_plength = str.length();
		memcpy(buffer->_payload, str.c_str(), buffer->_plength);
		return buffer;
	}

	static StreamBuffer<uint8_t, I>* fromPayload(uint8_t *payload, uint8_t plength) {
		return (StreamBuffer<uint8_t, I>*)StreamBufferGeneric<uint8_t, I>::fromPayload(payload, plength);
	}

	void serialize(uint8_t* buffer, uint16_t length) const {
		if (length < 3) return; // throw error

		uint8_t *ptr = buffer;
		*ptr++ = StreamBufferGeneric<uint8_t, I>::_type;
		*ptr++ = StreamBufferGeneric<uint8_t, I>::_plength;

		if (StreamBufferGeneric<uint8_t, I>::_plength) memcpy(ptr, StreamBufferGeneric<uint8_t, I>::_payload, StreamBufferGeneric<uint8_t, I>::_plength * sizeof(uint8_t));

	//	LOGd("serialize...");
	//	BLEutil::printArray(buffer, length);
	}

	void deserialize(uint8_t* buffer, uint16_t length) {
		if (length < 3) return;

		uint8_t *ptr = buffer;
		StreamBufferGeneric<uint8_t, I>::_type = *ptr++;
		StreamBufferGeneric<uint8_t, I>::_plength = *ptr++;

		StreamBufferGeneric<uint8_t, I>::init(StreamBufferGeneric<uint8_t, I>::_plength);
		memcpy(StreamBufferGeneric<uint8_t, I>::_payload, ptr, StreamBufferGeneric<uint8_t, I>::_plength * sizeof(uint8_t));

	//	LOGd("deserialize...");
	//	BLEutil::printArray(_payload, _plength);
	}
};
