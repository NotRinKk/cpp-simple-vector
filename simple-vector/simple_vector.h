#pragma once

#include <cassert>
#include <stdexcept>
#include <initializer_list>
#include "array_ptr.h"

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t capacity_to_reserve)
    : capacity_to_reserve_(capacity_to_reserve) {
    }

    size_t GetCapacity() const {
        return capacity_to_reserve_;
    }

private:
    size_t capacity_to_reserve_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}; 


template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
    : SimpleVector(size, Type{}) {
    }

    SimpleVector(ReserveProxyObj reserve_proxy)
    :  size_(0), capacity_(reserve_proxy.GetCapacity()), data_(nullptr) {
    }
    

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, Type value)
    : size_(size), capacity_(size), data_(size) {
        for(auto it = this->begin(); it != this->end(); ++it){
            *it = std::move(value);
        }
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
    : size_(init.size()), capacity_(init.size()), data_(init.size()) {
        std::copy(std::make_move_iterator(init.begin()),
                  std::make_move_iterator(init.end()),
                  data_.Get());
    }

    SimpleVector(const SimpleVector& other)
    : size_(other.size_), capacity_(other.capacity_), data_(other.capacity_) {
        std::copy(std::make_move_iterator(other.begin()),
                  std::make_move_iterator(other.end()), 
                  data_.Get());
    }

    // Конструктор перемещения
    SimpleVector(SimpleVector&& other) noexcept 
        : size_(other.size_), capacity_(other.capacity_), data_(std::move(other.data_)) {
        other.size_ = 0;
        other.capacity_ = 0;
    }

    // Оператор присваивания перемещением
    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
        if (this != &rhs) {
            if (rhs.IsEmpty()) {
                Clear();
                return *this;
            }
            SimpleVector tmp(std::move(rhs));
            swap(tmp);
        }
        return *this;
    }
    
    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            auto rhs_copy(rhs);
            swap(rhs_copy);
        }

        return *this;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index is out pf range!");
        }
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
         if (index >= size_) {
            throw std::out_of_range("Index is out pf range!");
        }
        return data_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0u;
    }

    // Задает ёмкость вектора. 
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            SimpleVector<Type> new_vector(new_capacity);
            new_vector.size_ = size_;
            std::copy(std::make_move_iterator(this->begin()),
                      std::make_move_iterator(this->end()),
                      new_vector.begin());
            swap(new_vector);
        }
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        // Если уменьшение размера, то уменьшем только размер контейнера
        if (new_size == size_) return;
        if (new_size < size_) {
            size_ = new_size;
            return;
        }

        // Если увеличение размера контейнера
        // то либо дозаполняем контейнер
        if (new_size > size_ && new_size <= capacity_) {
            for(auto it = (this->begin() + size_); it != (this->begin() + new_size); ++it){
                *it = std::move(Type{});
            }
            size_ = new_size;
        }
        // либо создаём новый массив и производим копирование элементов
        size_t capacity = std::max(new_size, capacity_ * 2);
        ArrayPtr<Type> new_arr(capacity); 
        
        for(auto it = (new_arr.Get() + size_); it != (new_arr.Get() + new_size); ++it){
            *it = std::move(Type{});
        }
        std::copy(std::make_move_iterator(this->begin()),
                  std::make_move_iterator(this->end()),
                  new_arr.Get());
        size_ = new_size;
        capacity_ = capacity;
        data_.swap(new_arr);
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(Type item) {
        if (size_ == 0u && capacity_ == 0u) {
            capacity_ = 1;
            ArrayPtr<Type> new_arr(capacity_);
            new_arr[size_] = std::move(item);
            ++size_;
            data_.swap(new_arr);
            return;
        }
        
        // Если контейнер полностью не заполнен
        if (size_ != capacity_) {
            data_[size_] = std::move(item);
            ++size_;
            return;
        }
        // Если контейнер заполнен полностью
        Reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        // Добавляем элемент
        data_[size_] = std::move(item);
        ++size_;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, Type value) {
        auto index = std::distance(this->cbegin(), pos);

        if (size_ == 0u && capacity_ == 0u) {
            if (index != 0) {
                return this->end();
            }
            // Резервируем место для одного элемента
            Reserve(1);
            // Вставляем элемент
            data_[index] = std::move(value); 
            ++size_;
            return &data_[index];
        }
        // Если контейнер полностью не заполнен
        if (size_ != capacity_) {
            std::copy_backward(std::make_move_iterator(this->begin() + index),
                               std::make_move_iterator(this->end()),
                               (this->end() + 1));
            data_[index] = std::move(value);
            ++size_;
            return &data_[index];
        }
        // Если вектор полностью заполнился
        Reserve(capacity_ * 2); // Резервируем в два раза больше места

        // Сдвигаем элементы после индекса вправо
        std::copy_backward(std::make_move_iterator(this->begin() + index),
                           std::make_move_iterator(this->end()),
                           this->end() + 1);
        data_[index] = std::move(value); // Вставляем новый элемент
        // Увеличиваем размер
        ++size_;
        return &data_[index];
    }

     // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (size_ == 0u) {
            return;
        }
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        auto index = std::distance(this->cbegin(), pos);
        std::copy(std::make_move_iterator(this->begin() + index + 1),
                 std::make_move_iterator(this->end()), this->begin() + index);
        --size_;
        return &data_[index];
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        data_.swap(other.data_);
    }


    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return data_.Get() + size_;
    }
private:
    size_t size_ = 0u;
    size_t capacity_ = 0u;
    ArrayPtr<Type> data_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize()
        && std::equal(lhs.begin(), lhs.end(),
                      rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                        rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs > rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
} 