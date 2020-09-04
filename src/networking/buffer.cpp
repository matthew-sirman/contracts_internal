//
// Created by Matthew on 03/09/2020.
//

#include "../../include/networking/buffer.h"

byte_buffer::byte_buffer()
        : __buffer(nullptr), __size(0) {

}

byte_buffer::byte_buffer(size_t n)
        : __buffer(std::make_unique<byte[]>(n)), __size(n) {

}

byte_buffer::byte_buffer(byte_buffer &&other) noexcept
        : __buffer(std::move(other.__buffer)), __size(other.__size) {
    other.__size = 0;
}

byte_buffer::byte_buffer(nullptr_t)
        : byte_buffer() {

}

byte_buffer::~byte_buffer() = default;

byte_buffer &byte_buffer::operator=(byte_buffer &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->__buffer = std::move(other.__buffer);
    this->__size = other.__size;
    other.__size = 0;

    return *this;
}

byte_buffer &byte_buffer::operator=(nullptr_t) {
    this->__buffer = nullptr;
    this->__size = 0;

    return *this;
}

byte_buffer::operator bool() const {
    return static_cast<bool>(__buffer);
}

byte_buffer::buffer_t *byte_buffer::operator->() {
    return &__buffer;
}

const byte_buffer::buffer_t *byte_buffer::operator->() const {
    return &__buffer;
}

byte *byte_buffer::begin() {
    return __buffer.get();
}

byte *byte_buffer::end() {
    return __buffer.get() + __size;
}

const byte *byte_buffer::cbegin() const {
    return __buffer.get();
}

const byte *byte_buffer::cend() const {
    return __buffer.get() + __size;
}

byte_buffer byte_buffer::copy() const {
    byte_buffer __copy(this->__size);
    std::copy(cbegin(), cend(), __copy.begin());
    return std::move(__copy);
}

shared_byte_buffer::shared_byte_buffer()
        : __buffer(nullptr) {

}

shared_byte_buffer::shared_byte_buffer(size_t n)
        : __buffer(std::make_shared<std::vector<byte>>(n)) {

}

shared_byte_buffer::shared_byte_buffer(const shared_byte_buffer &other)
    : __buffer(other.__buffer) {

}

shared_byte_buffer::shared_byte_buffer(shared_byte_buffer &&other) noexcept
        : __buffer(std::move(other.__buffer)) {

}

shared_byte_buffer::shared_byte_buffer(nullptr_t)
        : shared_byte_buffer() {

}

shared_byte_buffer::~shared_byte_buffer() = default;

shared_byte_buffer &shared_byte_buffer::operator=(const shared_byte_buffer &other) {
    if (this == &other) {
        return *this;
    }

    this->__buffer = other.__buffer;

    return *this;
}

shared_byte_buffer &shared_byte_buffer::operator=(shared_byte_buffer &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->__buffer = std::move(other.__buffer);

    return *this;
}

shared_byte_buffer &shared_byte_buffer::operator=(nullptr_t) {
    this->__buffer = nullptr;

    return *this;
}

shared_byte_buffer::operator bool() const {
    return static_cast<bool>(__buffer);
}

shared_byte_buffer::buffer_t *shared_byte_buffer::operator->() {
    return &__buffer;
}

const shared_byte_buffer::buffer_t *shared_byte_buffer::operator->() const {
    return &__buffer;
}

byte *shared_byte_buffer::begin() {
    return __buffer->data();
}

byte *shared_byte_buffer::end() {
    return __buffer->data() + __buffer->size();
}

const byte *shared_byte_buffer::cbegin() const {
    return __buffer->data();
}

const byte *shared_byte_buffer::cend() const {
    return __buffer->data() + __buffer->size();
}

byte_buffer shared_byte_buffer::uniqueCopy() const {
    byte_buffer __copy(__buffer->size());
    std::copy(__buffer->begin(), __buffer->end(), __copy.begin());
    return std::move(__copy);
}

size_t shared_byte_buffer::size() const {
    return __buffer->size();
}
