#pragma once

typedef struct {
  void* head;
  long long int space;
} allocator;

void* alloc(allocator* a, long long int bytes);
