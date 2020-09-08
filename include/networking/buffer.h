//
// Created by Matthew on 03/09/2020.
//

#ifndef CONTRACTS_INTERNAL_BUFFER_H
#define CONTRACTS_INTERNAL_BUFFER_H

#include <memory>
#include <vector>

using byte = unsigned char;

struct byte_buffer {
public:
    using buffer_t = std::unique_ptr<byte[]>;

    byte_buffer();

    byte_buffer(size_t n);

    byte_buffer(const byte_buffer &other) = delete;

    byte_buffer(byte_buffer &&other) noexcept;

    byte_buffer(nullptr_t);

    ~byte_buffer();

    byte_buffer &operator=(const byte_buffer &other) = delete;

    byte_buffer &operator=(byte_buffer &&other) noexcept;

    byte_buffer &operator=(nullptr_t);

    operator bool() const;

    buffer_t *operator->();

    const buffer_t *operator->() const;

    byte *begin();

    byte *end();

    const byte *cbegin() const;

    const byte *cend() const;

    byte_buffer copy() const;

    constexpr size_t size() const {
        return __size;
    }

private:
    buffer_t __buffer;
    size_t __size;
};

struct shared_byte_buffer {
public:
    using buffer_t = std::shared_ptr<std::vector<byte>>;

    shared_byte_buffer();

    shared_byte_buffer(size_t n);

    shared_byte_buffer(const shared_byte_buffer &other);

    shared_byte_buffer(shared_byte_buffer &&other) noexcept;

    shared_byte_buffer(nullptr_t);

    ~shared_byte_buffer();

    shared_byte_buffer &operator=(const shared_byte_buffer &other);

    shared_byte_buffer &operator=(shared_byte_buffer &&other) noexcept;

    shared_byte_buffer &operator=(nullptr_t);

    operator bool() const;

    buffer_t *operator->();

    const buffer_t *operator->() const;

    byte *begin();

    byte *end();

    const byte *cbegin() const;

    const byte *cend() const;

    byte_buffer uniqueCopy() const;

    void resize(size_t n);

    size_t size() const;

private:
    buffer_t __buffer;
};


#endif //CONTRACTS_INTERNAL_BUFFER_H
