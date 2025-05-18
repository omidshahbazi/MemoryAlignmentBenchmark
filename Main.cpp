#include <memory>
#include <chrono>

using namespace std;
using namespace chrono;

const uint32_t NUM_ELEMENTS = 100'000'000;

template<uint64_t Alignment = 0>
void* Allocate(uint32_t Size)
{
	static_assert(Alignment == 0 || (Alignment & (Alignment - 1)) == 0, "Alignment must be zero or power of two");

	Size += Alignment + sizeof(void*);

	printf("Total allocation size: %d bytes\n", Size);

	char* ptr = reinterpret_cast<char*>(malloc(Size));
	if (ptr == nullptr)
		return nullptr;

	printf("Original address: %p\n", ptr);

	char* originalPtr = ptr;

	if constexpr (Alignment != 0)
	{
		char* alignedPtr = ptr + sizeof(void*);

		alignedPtr = reinterpret_cast<char*>((reinterpret_cast<uintptr_t>(alignedPtr) + (Alignment - 1)) & ~(Alignment - 1));

		printf("Aligned of %d address: %p\n", Alignment, alignedPtr);

		ptr = alignedPtr - sizeof(void*);
	}

	*reinterpret_cast<char**>(ptr) = originalPtr;
	printf("Stored original pointer at: %p\n", ptr);

	ptr += sizeof(void*);

	printf("Data pointer at: %p\n", ptr);

	return ptr;
}

template<typename DataType, uint32_t Alignment = 0>
DataType* Allocate(uint32_t Length = 1)
{
	return reinterpret_cast<DataType*>(Allocate<Alignment>(Length * sizeof(DataType)));
}

template<typename DataType>
DataType* AlignedAllocate(uint32_t Length = 1)
{
	return reinterpret_cast<DataType*>(Allocate<alignof(DataType)>(Length * sizeof(DataType)));
}

void Deallocate(void* Pointer)
{
	char* ptr = reinterpret_cast<char*>(Pointer);
	ptr -= sizeof(void*);

	ptr = *reinterpret_cast<char**>(ptr);

	printf("Deallocating original address: %p\n", ptr);

	free(ptr);
}

template<uint32_t Alignment>
nanoseconds Benchmark()
{
	printf("=========================================================\n");

	if constexpr (Alignment == 0)
		printf("Benchmarking Unaligned\n");
	else
		printf("Benchmarking alignment %i\n", Alignment);

	uint32_t* ptr = Allocate<uint32_t, Alignment>(NUM_ELEMENTS);

	//Warm up just to flush the data from the cache
	for (uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		ptr[i] = -1;

	time_point startPoint = high_resolution_clock::now();

	for (uint32_t i = 0; i < NUM_ELEMENTS; ++i)
		ptr[i] = 100;

	nanoseconds elapsedTime = high_resolution_clock::now() - startPoint;

	Deallocate(ptr);

	printf("TotalTime: %dns, Per Access Time: %fns\n", elapsedTime.count(), (double)elapsedTime.count() / NUM_ELEMENTS);
	printf("=========================================================\n");

	return elapsedTime;
}

int main()
{
	const uint32_t ALIGNMENT = 32;

	nanoseconds unalignedElapsedTime = Benchmark<0>();
	nanoseconds alignedElapsedTime = Benchmark<ALIGNMENT>();

	printf("Unaligned memory access is %f%% slower than alignement of %i", ((unalignedElapsedTime.count() / (double)alignedElapsedTime.count()) - 1) * 100, ALIGNMENT);
}