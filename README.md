# List and Stack Allocator
В этой задаче вам предлагается научиться пользоваться аллокаторами, а также разобраться с устройством контейнера list и понять, что иногда нестандартный аллокатор может давать выигрыш в производительности по сравнению со стандартным.

## Часть 1.

Напишите класс *StackAllocator<typename T, size_t N>*, который бы позволял создавать стандартные контейнеры на стеке, без единого обращения к динамической памяти. Для этого вам пригодится написать класс *StackStorage<size_t N>*, в котором будет храниться большой массив на стеке. Объект класса *StackAllocator* должен быть легковесным, легко копироваться и присваиваться. Объект *StackStorage*, напротив, не должен поддерживать копирование.

Класс *StackAllocator* должен удовлетворять требованиям к аллокатору, описанным на странице https://en.cppreference.com/w/cpp/named_req/Allocator. Он должен быть *STL*-совместимым, то есть позволять использование в качестве аллокатора для стандартных контейнеров. В частности, должны быть определены:
- Конструктор по умолчанию, конструктор копирования, деструктор, оператор присваивания;
- Методы *allocate, deallocate*;
- Внутренний тип *value_type*;
- Метод *select_on_container_copy_construction*, если логика работы вашего аллокатора этого потребует.
Пример того, как может использоваться *StackAllocator*:
```cpp
int main() { 
StackStorage<100'000> storage;
StackAllocator<int, 100'000> alloc(storage);
std::vector<int, StackAllocator<int, 100'000» v(alloc);
// ... useful stuff ...
}
```
Во время исполнения может существовать несколько *StackStorage* с одним и тем же *N*. Разумеется, аллокаторы, построенные на разных *StackStorage*, должны считаться неравными.

*StackAllocator* должен заботиться о выравнивании объектов. В частности, нельзя класть переменные типа *int* по адресам, не кратным 4; переменные типа *double* - по адресам, не кратным 8, и т.д..

Использование любого контейнера со *StackAllocator* вместо *std::allocator* должно приводить к тому, что обращений к динамической памяти в программе не происходит вообще.

Проверьте себя: напишите тестирующую функцию, которая создает *std::list* и выполняет над ним последовательность случайных добавлений/удалений элементов. Если вы все сделали правильно, то *std::list* со *StackAllocator*'ом должен работать быстрее, чем *std::list* со стандартным аллокатором.

## Часть 2.

Напишите класс *List* - двусвязный список с правильным использованием аллокатора. Правильное использование аллокатора означает, что ваш лист должен удовлетворять требованиям https://en.cppreference.com/w/cpp/named_req/AllocatorAwareContainer. Должно быть два шаблонных параметра: 
*T* - тип элементов в листе, *Allocator* - тип используемого аллокатора (по умолчанию - *std::allocator<T>*).

Должны быть поддержаны:

- Конструкторы:
  + без параметров;
  + от одного числа;
  + от числа и *const T&*;
  + от одного аллокатора;
  + от числа и аллокатора;
  + от числа, *const T&* и аллокатора.
- Метод *get_allocator()*, возвращающий объект аллокатора, используемый в листе на данный момент;
- Конструктор копирования, деструктор, копирующий оператор присваивания;
- Метод *size()*, работающий за *O(1)*;
- Методы *push_back, push_front, pop_back, pop_front*;
- Двунаправленные итераторы, удовлетворяющие требованиям https://en.cppreference.com/w/cpp/named_req/BidirectionalIterator. Также поддержите константные и *reverse*-итераторы;
- Методы *begin, end, cbegin, cend, rbegin, rend, crbegin, crend;*
- Методы *insert(iterator, const T&)*, а также *erase(iterator)* - для удаления и добавления одиночных элементов в список.
Все методы листа должны быть строго безопасны относительно исключений. Это означает, что при выбросе исключения из любого метода класса *T* во время произвольной операции *X* над листом лист должен вернуться в состояние, которое было до начала выполнения *X*, не допустив *UB* и утечек памяти, и пробросить исключение наверх в вызывающую функцию. Можно считать, что конструкторы и операторы присваивания у аллокаторов исключений никогда не кидают (это является частью требований к *Allocator*).

Как ваш собственный *List*, так и *std::list* должен показывать более высокую производительность со StackAllocator’ом, чем со стандартным аллокатором.
