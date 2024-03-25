#pragma once

#include <vector>
#include <string>
#include <cassert>
#include <algorithm>
#include <array>

namespace logging {

std::string byteArrayToString(const void* buffer, size_t length);

/**
 * A vector-like class which uses a statically allocated buffer if the data is small enough.
 */
class ByteArray {

public:
	ByteArray() {
	}

	ByteArray(const ByteArray& b) {
		// duplicate content
		append( b.getData(), b.size() );
	}

	ByteArray& operator=(const ByteArray& right) {
		resize( right.size() );
		std::copy_n(right.getData(), right.size(), getData());
		return *this;
	}

	~ByteArray() {
		if ( !usesStaticBuffer() )
			delete (m_dynamicData);
	}

	size_t size() const {
		return ( usesStaticBuffer() ? m_length : m_dynamicData->size() );
	}

	bool usesStaticBuffer() const {
		return (m_dynamicData == nullptr);
	}

	char* getData() {
		return usesStaticBuffer() ? m_staticData.data() : m_dynamicData->data();
	}

	const char* getData() const {
		return usesStaticBuffer() ? m_staticData.data() : m_dynamicData->data();
	}

	char& operator[](size_t index) {
		assert(index < m_length);
		return getData()[index];
	}

	void resize(size_t size) {

		if ( !usesStaticBuffer() ) {
			m_dynamicData->resize(size);
		} else {
			if ( size <= sizeof(m_staticData) ) {
				m_length = size;
			} else {
				m_dynamicData = new std::vector<char>();
				m_dynamicData->resize(size);
				std::copy_n(m_staticData.begin(), m_length, m_dynamicData->data());
				m_length = -1; // this field is not relevant anymore
			}
		}

	}

	void writeAt(size_t position, const void* rawDataPtr, size_t sizeInByte) {
		assert(m_length >= position + sizeInByte);
		std::copy_n(reinterpret_cast<const char*>(rawDataPtr), sizeInByte, getData() + position);
	}

	void append(const void* rawDataPtr, size_t sizeInByte) {
		auto i = size();
		resize(size() + sizeInByte);
		std::copy_n(reinterpret_cast<const char*>(rawDataPtr), sizeInByte, getData() + i);
	}

	void append(unsigned char v) {
		auto i = size();
		resize(i + 1);
		getData()[i] = v;
	}

	size_t capacity() const {
		if ( usesStaticBuffer() )
			return sizeof(m_staticData);
		else
			return m_dynamicData->size();
	}

	std::string toString() const {
		return byteArrayToString( getData(), size() );
	}

private:
	std::vector<char>* m_dynamicData = nullptr;
	std::array<char, 512> m_staticData;
	size_t m_length = 0;
};

typedef ByteArray ResizeableByteArray;

}
