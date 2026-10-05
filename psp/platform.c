/* Leave room for SDL, the 640x200 raster and its audio buffers in PSP RAM. */
unsigned int _newlib_heap_size_user=8*1024*1024;
unsigned int _main_thread_stack_size=512*1024;
