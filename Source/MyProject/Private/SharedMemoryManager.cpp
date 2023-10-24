// Fill out your copyright notice in the Description page of Project Settings.


#include "SharedMemoryManager.h"

SharedMemoryManager::SharedMemoryManager()
{
	bip::shared_memory_object::remove("MySharedMemory");
	segment = new bip::managed_shared_memory(bip::open_or_create, "MySharedMemory", 921600 * 10);
	char_alloc = new shm::char_alloc(segment->get_segment_manager());
	queue = segment->construct<shm::ring_buffer>("queue")();
}

SharedMemoryManager::~SharedMemoryManager()
{
	delete queue;
	delete char_alloc;
	bip::shared_memory_object::remove("MySharedMemory");
}

void SharedMemoryManager::push_frame(char* frame, size_t frameSize)
{
	shm::shared_vector vector(*char_alloc);
	vector.insert(vector.begin(), frame, frame + frameSize);
	queue->push(vector);
}
