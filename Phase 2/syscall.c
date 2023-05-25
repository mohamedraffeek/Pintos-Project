#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include <string.h>
#include <stdlib.h>
#include "syscall.h"
#include "threads/synch.h"
static struct lock lock;

void syscall_init(void);
void validate_void_ptr(const void *ptr);
static void syscall_handler(struct intr_frame *);
void halt(void);
void sys_exit(int status);
tid_t sys_exec(char *cmd_line);
int sys_wait(int pid);
bool sys_create(char *file, unsigned initial_size);
bool sys_remove(char *file);
int sys_open(char *file_name);
int file_size(int fd);
int sys_write(int fd, void *buffer, int size);
int sys_read(int fd, void *buffer, int size);
void sys_seek(int fd, unsigned position);
unsigned sys_tell(int fd);
void sys_close(int fd);
struct file *get_file(int fd);
void remove_file(int fd);
static int generate_fd();

static void syscall_handler (struct intr_frame *);

void syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&lock);
}

// check if the validation of the pointer
void validate_void_ptr(const void* pt){
  if (pt == NULL || !is_user_vaddr(pt) || pagedir_get_page(thread_current()->pagedir, pt) == NULL)
    sys_exit(-1);
}


static void
syscall_handler (struct intr_frame *f) {
  printf ("system call!\n");
  
  //first get Int from the stack
  void *esp = f->esp;
	switch (*(int *)esp){

    case SYS_HALT:
      halt();
      break;

    case SYS_EXIT:
      validate_void_ptr(esp + 4);
      // get the the exit status
		  int status = *((int *)esp + 1);
		  sys_exit(status);
		  break;

    case SYS_WAIT:
		  validate_void_ptr(esp + 4);
		  int pid = (*((int *)esp + 1));
		  f->eax = sys_wait(pid);
	    break; 

    case SYS_EXEC:
      validate_void_ptr(esp + 4);

      char *cmd_line = (char *)(*((int *)esp + 1));
      if (cmd_line == NULL) sys_exit(-1);

      lock_acquire(&lock);
      f->eax = sys_exec(cmd_line);
      lock_release(&lock);

      break;

    case SYS_CREATE:
		  validate_void_ptr(esp + 4);
		  validate_void_ptr(esp + 8);
      char *file;
		  file = (char *)(*((uint32_t *)esp + 1));
		  unsigned init_size = *((unsigned *)esp + 2);
      //check if the file doesnot exist
      if (file == NULL) 
        sys_exit(-1);
		  f->eax = sys_create(file, init_size);
	    break;  
  
    case SYS_REMOVE:
		  validate_void_ptr(esp + 4);
      char *file;
		  file = (char *)(*((uint32_t *)esp + 1));
      //check if the file doesnot exist
      if (file == NULL) 
        sys_exit(-1);
		  f->eax = sys_remove(file);
	    break;

    case SYS_OPEN:
		  validate_void_ptr(esp + 4);
		  char *file_name = (char *)(*((uint32_t *)esp + 1));
      //check if the file doesnot exist
      if (file_name == NULL) 
        sys_exit(-1);
		  f->eax = sys_open(file_name);
	    break;

    case SYS_FILESIZE:
		  validate_void_ptr(esp + 4);
		  int fd = *((uint32_t *)esp + 1);
		  f->eax = file_size(fd);
	    break;
 
    case SYS_READ:
		  validate_void_ptr(esp + 4);
		  validate_void_ptr(esp + 8);
		  validate_void_ptr(esp + 12);

      int fd = *((int*)f->esp + 1);
      void *buffer = (void*)(*((int*)f->esp + 2));
      int size = *((int*)f->esp + 3);

      validate_void_ptr(buffer+size);

		  f->eax = sys_read(fd, buffer, size);
	    break;

    case SYS_WRITE:
		  validate_void_ptr(esp + 4);
		  validate_void_ptr(esp + 8);
		  validate_void_ptr(esp + 12);
		
      int fd = *((uint32_t *)esp + 1);
		  void *buffer = (void *)(*((uint32_t *)esp + 2));
		  int size = *((unsigned *)esp + 3);
      if(buffer==NULL) sys_exit(-1);

		  f->eax = sys_write(fd, buffer, size);
      break;
  
    case SYS_SEEK:
		  validate_void_ptr(esp + 4);
		  validate_void_ptr(esp + 8);
		  int fd = *((uint32_t *)esp + 1);
		  int pos = (*((unsigned *)esp + 2));
		  sys_seek(fd, pos);
	    break;

    case SYS_TELL:
      validate_void_ptr(esp + 4);
      int fd = *((uint32_t *)esp + 1);
      f->eax = sys_tell(fd);
      break;

    case SYS_CLOSE:
      validate_void_ptr(esp + 4);
      int fd = *((uint32_t *)esp + 1);
      sys_close(fd);
      break;
  
    default:
      break;
  }

}

void halt(void){shutdown_power_off();}

void sys_exit(int status){

	struct thread *cur = thread_current()->parent;
	printf("%s: exit(%d)\n", thread_current()->name, status);
	if(cur) cur->childStatus = status;
	thread_exit();
}

tid_t sys_exec(char *cmd_line){
	return process_execute(cmd_line);
}

int sys_wait(int pid){return process_wait(pid);}

bool sys_create(char *file, unsigned initial_size){
	lock_acquire(&lock);
	bool created = filesys_create(file, initial_size);
	lock_release(&lock);
	return created;
}

bool sys_remove(char *file){
	lock_acquire(&lock);
	bool removed = filesys_remove(file);
	lock_release(&lock);
	return removed;
}

int file_size(int fd){
	struct file *file = get_file(fd)->ptr;
	
  if (file == NULL)
		return -1;
	
	lock_acquire(&lock);
	int fileSize = file_length(file);
	lock_release(&lock);
	return fileSize;
}


int sys_read(int fd, void *buffer, int length){
  if (fd == 0){  //input stream
    
    for (size_t i = 0; i < length; i++){
      lock_acquire(&lock);
      ((char*)buffer)[i] = input_getc();
      lock_release(&lock);
    }
    return length;
    
  }
  else if (fd == 1){ //output stream
    //negative area cant happened cuz the validation that happened before
    printf("negative area : output stream in read call");
  }
  else {
    struct file* my_file = get_file(fd)->ptr;

    if (my_file == NULL){return -1;}

    int res;
    lock_acquire(&lock);
    res = file_read(my_file,buffer,length);
    lock_release(&lock);
    return res;
  }
}

int sys_write(int fd, void *buffer, int length){
	int size_written = 0;

	if (fd == 1){ /// output stream
		lock_acquire(&lock);
		putbuf(buffer, length);
		size_written = (int)length;
		lock_release(&lock);
	}
	else if (fd == 0){ //input stream
    //negative area cant happened cuz the validation that happened before
    printf("negative area : input stream in putput call");
  }
  else{ //// writing normally to an open file
		
    struct file* my_file = get_file(fd)->ptr;

    if (my_file == NULL){return -1;}

		lock_acquire(&lock);
		size_written = (int)file_write(my_file, buffer, length);
		lock_release(&lock);
	}
	return size_written;
}

void sys_seek(int fd, unsigned position){
	struct file *my_file = get_file(fd)->ptr;
	if (my_file == NULL || position < 0) return;

	lock_acquire(&lock);
	file_seek(my_file, position);
	lock_release(&lock);
}

// get the pos of next file to be read or written in an open file
unsigned sys_tell(int fd){
	struct file *my_file = get_file(fd)->ptr;
	if (my_file == NULL) return -1;

	lock_acquire(&lock);
	int pos = (int)file_tell(ft);
	lock_release(&lock);
	return pos;
}

int sys_open(char *file_name){	
  struct file* openFile = palloc_get_page(0);
  if (openFile == NULL) 
  {
    palloc_free_page(openFile);
    return -1;
  }

  lock_acquire(&lock);
  openFile->ptr = filesys_open(file_name);
  lock_release(&lock);
  
  if (openFile->ptr == NULL) return -1;
  
  openFile->fd = ++thread_current()->fileDir;
  list_push_back(&thread_current()->filesList,&openFile->elem);
  return openFile->fd;
}

void sys_close(int fd){
	struct file *my_file = get_file(fd);
	if (my_file == NULL) return;

  lock_acquire(&lock);
  file_close(my_file->ptr);
  lock_release(&lock);
  list_remove(&my_file->elem);
  palloc_free_page(my_file);
}

struct file* get_file(int fd){

    struct thread* t = thread_current();
    struct file* my_file = NULL;

    // go throught open files in that thread and return fd if founded
    for (struct list_elem* e = list_begin (&t->filesList); e != list_end (&t->filesList);
    e = list_next (e)){
      
      struct file* opened_file = list_entry (e, struct file, elem);
      if (opened_file->fd == fd)
        return opened_file;
      
    }
    return NULL;
}
