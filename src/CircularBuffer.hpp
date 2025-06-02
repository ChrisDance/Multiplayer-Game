#pragma once

#include <stdexcept>
#include <cstddef>
#include <optional>

template <typename T>
class CircularBuffer
{
private:
    T *buffer;
    size_t capacity;
    size_t size_;
    size_t head;
    size_t tail;

public:
    explicit CircularBuffer(size_t capacity)
        : capacity(capacity), size_(0), head(0), tail(0)
    {
        if (capacity == 0)
        {
            throw std::invalid_argument("Capacity must be greater than 0");
        }
        buffer = new T[capacity];
    }

    CircularBuffer(const CircularBuffer &other)
        : capacity(other.capacity), size_(other.size_), head(other.head), tail(other.tail)
    {
        buffer = new T[capacity];
        for (size_t i = 0; i < capacity; ++i)
        {
            buffer[i] = other.buffer[i];
        }
    }

    CircularBuffer(CircularBuffer &&other) noexcept
        : buffer(other.buffer), capacity(other.capacity), size_(other.size_),
          head(other.head), tail(other.tail)
    {
        other.buffer = nullptr;
        other.capacity = 0;
        other.size_ = 0;
        other.head = 0;
        other.tail = 0;
    }

    CircularBuffer &operator=(const CircularBuffer &other)
    {
        if (this != &other)
        {
            delete[] buffer;

            capacity = other.capacity;
            size_ = other.size_;
            head = other.head;
            tail = other.tail;

            buffer = new T[capacity];
            for (size_t i = 0; i < capacity; ++i)
            {
                buffer[i] = other.buffer[i];
            }
        }
        return *this;
    }

    CircularBuffer &operator=(CircularBuffer &&other) noexcept
    {
        if (this != &other)
        {
            delete[] buffer;

            buffer = other.buffer;
            capacity = other.capacity;
            size_ = other.size_;
            head = other.head;
            tail = other.tail;

            other.buffer = nullptr;
            other.capacity = 0;
            other.size_ = 0;
            other.head = 0;
            other.tail = 0;
        }
        return *this;
    }

    ~CircularBuffer()
    {
        delete[] buffer;
    }

    void push(const T &item)
    {
        buffer[tail] = item;

        if (size_ < capacity)
        {

            size_++;
        }
        else
        {

            head = (head + 1) % capacity;
        }

        tail = (tail + 1) % capacity;
    }

    void push(T &&item)
    {
        buffer[tail] = std::move(item);

        if (size_ < capacity)
        {

            size_++;
        }
        else
        {

            head = (head + 1) % capacity;
        }

        tail = (tail + 1) % capacity;
    }

    T pop()
    {
        if (empty())
        {
            throw std::underflow_error("Cannot pop from an empty buffer");
        }

        T item = buffer[head];
        head = (head + 1) % capacity;
        size_--;

        return item;
    }

    T &front() const
    {
        if (empty())
        {
            throw std::underflow_error("Buffer is empty");
        }
        return buffer[head];
    }

    const T &back() const
    {
        if (empty())
        {
            throw std::underflow_error("Buffer is empty");
        }
        size_t backIndex = (tail == 0) ? capacity - 1 : tail - 1;
        return buffer[backIndex];
    }

    T &at(size_t index)
    {
        if (index >= size_)
        {
            throw std::out_of_range("Index out of bounds");
        }
        size_t actual_index = (head + index) % capacity;
        return buffer[actual_index];
    }

    const T &at(size_t index) const
    {
        if (index >= size_)
        {
            throw std::out_of_range("Index out of bounds");
        }
        size_t actual_index = (head + index) % capacity;
        return buffer[actual_index];
    }

    bool empty() const
    {
        return size_ == 0;
    }

    bool full() const
    {
        return size_ == capacity;
    }

    size_t size() const
    {
        return size_;
    }

    size_t max_size() const
    {
        return capacity;
    }

    void clear()
    {
        head = 0;
        tail = 0;
        size_ = 0;
    }
};
