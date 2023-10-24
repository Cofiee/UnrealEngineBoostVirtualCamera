// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#pragma push_macro("check")
#undef check
#pragma push_macro("verify")
#undef verify

#include <boost/lockfree/spsc_queue.hpp> // ring buffer
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/vector.hpp>

#pragma pop_macro("verify")
#pragma pop_macro("check")
THIRD_PARTY_INCLUDES_END

/**
 *
 */
namespace bip = boost::interprocess;
namespace shm
{
    typedef bip::allocator<char, bip::managed_shared_memory::segment_manager> char_alloc;
    typedef bip::vector<char, char_alloc> shared_vector;
    typedef boost::lockfree::spsc_queue<
        shared_vector,
        boost::lockfree::capacity<2>
    > ring_buffer;
}

class MYPROJECT_API SharedMemoryManager
{
private:
    bip::managed_shared_memory* segment;
    shm::char_alloc* char_alloc;
    shm::ring_buffer* queue;

public:
	SharedMemoryManager();
	~SharedMemoryManager();
    void push_frame(char* frame, size_t frameSize);
};
