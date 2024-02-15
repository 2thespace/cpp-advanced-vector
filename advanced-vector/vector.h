#pragma once

#include<algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>


template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }



    RawMemory(const RawMemory&) = delete;

    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept {
        buffer_ = other.buffer_;
        capacity_ = other.capacity_;
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept
    {
        if (this != &rhs) {
            RawMemory rhs_tmp(std::move(rhs));
            this->Swap(rhs_tmp);
        }
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Ðàçðåøàåòñÿ ïîëó÷àòü àäðåñ ÿ÷åéêè ïàìÿòè, ñëåäóþùåé çà ïîñëåäíèì ýëåìåíòîì ìàññèâà
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Âûäåëÿåò ñûðóþ ïàìÿòü ïîä n ýëåìåíòîâ è âîçâðàùàåò óêàçàòåëü íà íå¸
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Îñâîáîæäàåò ñûðóþ ïàìÿòü, âûäåëåííóþ ðàíåå ïî àäðåñó buf ïðè ïîìîùè Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept
    {
        return const_cast<iterator>(this->cbegin());
    }

    iterator end() noexcept
    {
        return const_cast<iterator>(this->cend());
    }

    const_iterator begin() const noexcept
    {
        return this->cbegin();
    }

    const_iterator end() const noexcept
    {
        return this->cend();
    }

    const_iterator cbegin() const noexcept
    {
        return data_.GetAddress();
    }
    const_iterator cend() const noexcept
    {
        return data_.GetAddress() + this->size_;
    }

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
    {
        this->Swap(other);
    }

    Vector& operator=(const Vector& rhs)
    {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                if (rhs.size_ < this->size_) {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                else {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + size_, data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                }
            }
            size_ = rhs.size_;
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    void Swap(Vector& other) noexcept
    {
        this->data_.Swap(other.data_);
        std::swap(this->size_, other.size_);
    }

    ~Vector()
    {
        std::destroy_n(data_.GetAddress(), size_);
    }


    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }


    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data = RawMemory<T>(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size)
    {
        if (new_size <= this->size_) {
            std::destroy_n(data_.GetAddress() + new_size, this->size_ - new_size);

        }
        else {
            this->Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - this->size_);
        }
        size_ = new_size;
    }

    template <typename Type>
    void PushBack(Type&& value)
    {
        if (size_ == this->Capacity()) {
            RawMemory<T> new_data(this->size_ == 0 ? 1 : this->size_ * 2);
            new(new_data.GetAddress() + this->size_) T(std::forward<Type>(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), this->size_);
            data_.Swap(new_data);
        }
        else {
            new(data_.GetAddress() + this->size_) T(std::forward<Type>(value));
        }

        size_++;
    }


    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == this->Capacity()) {
            RawMemory<T> new_data(this->size_ == 0 ? 1 : this->size_ * 2);
            new(new_data.GetAddress() + this->size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                try {
                    std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
                }
                catch (...)
                {
                    std::destroy_at(new_data.GetAddress() + this->size_);
                    throw;
                }
                
            }
            std::destroy_n(data_.GetAddress(), this->size_);
            data_.Swap(new_data);
        }
        else {
            new(data_.GetAddress() + this->size_) T(std::forward<Args>(args)...);
        }

        size_++;
        return *(data_.GetAddress() + this->size_ - 1);
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args)
    {
        assert(pos >= begin() && pos < end())
        std::size_t offset = std::distance(this->begin(), const_cast<iterator>(pos));
        if (this->size_ == this->Capacity()) {
            auto new_cap = this->size_ == 0 ? 1 : this->size_ * 2;
            RawMemory<T> new_data(new_cap);
            new(new_data.GetAddress() + offset) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), offset, new_data.GetAddress());
                std::uninitialized_move_n(data_.GetAddress() + offset, size_ - offset, new_data.GetAddress() + offset + 1);
            }
            else {
                try {
                    std::uninitialized_copy_n(data_.GetAddress(), offset, new_data.GetAddress());
                    std::uninitialized_copy_n(data_.GetAddress() + offset, size_ - offset, new_data.GetAddress() + offset + 1);
                }
                catch(...) {
                    std::destroy_at(new_data.GetAddress() + offset);
                    throw;
                }
            }
            std::destroy_n(data_.GetAddress(), this->size_);
            data_.Swap(new_data);
        }
        else
        {
            if (this->size_ != 0) {
                auto& last_value = *(this->end() - 1);
                new(this->end()) T(std::forward<T>(last_value));
                try {
                    std::move_backward(this->begin() + offset, this->end(), this->end() + 1);
                }
                catch(...) {
                    std::destroy_at(this->end());
                    throw;
                }
                std::destroy_at(this->begin() + offset);
            }
            new(this->begin() + offset) T(std::forward<Args>(args)...);
        
        }
        this->size_++;
        return  this->begin() + offset;
    }

    void PopBack() /* noexcept */
    {
        if (this->size_ > 0) {
            size_--;
            std::destroy_at(this->end());

        }
    }

    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/
    {
        assert(pos >= begin() && pos < end())
        auto offset = pos - this->begin();
        std::move(this->begin() + offset + 1, this->end(), this->begin() + offset);
        this->PopBack();
        return this->begin() + offset;
    }

    iterator Insert(const_iterator pos, const T& value)
    {
        //T tmp_value = value;
        return this->Emplace(pos, value);

    }

    iterator Insert(const_iterator pos, T&& value)
    {
      //  T tmp_value  = std::move(value);
        return this->Emplace(pos, std::move(value));

    }
private:
    // Âûäåëÿåò ñûðóþ ïàìÿòü ïîä n ýëåìåíòîâ è âîçâðàùàåò óêàçàòåëü íà íå¸
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Âûçûâàåò äåñòðóêòîðû n îáúåêòîâ ìàññèâà ïî àäðåñó buf
    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            Destroy(buf + i);
        }
    }

    // Ñîçäà¸ò êîïèþ îáúåêòà elem â ñûðîé ïàìÿòè ïî àäðåñó buf
    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }
    // Âûçûâàåò äåñòðóêòîð îáúåêòà ïî àäðåñó buf
    static void Destroy(T* buf) noexcept {
        buf->~T();
    }


private:
    RawMemory<T> data_;
    size_t size_ = 0;

};
