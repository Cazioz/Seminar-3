#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include "green.h"

#define FALSE 0
#define TRUE 1

#define STACK_SIZE 4096

green_t *first = NULL;
green_t *last = NULL;

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL,
  NULL, NULL, NULL, FALSE};

static green_t *running = &main_green;

static void init() __attribute__((constructor));

void init() {
  getcontext(&main_cntx);
}

/*void addToList(green_t *new) {
  if(first == NULL && last == NULL) {
    first = new;
    last = new;
    new->next = NULL;
  } else {
    new->next = first;
    first = new;
  }
}*/

void addLast (green_t *new) {
  if(first == NULL && last == NULL) {
    first = new;
    last = new;
    new->next = NULL;
  }
  else {
    last->next = new;
    last = new;
    new->next = NULL;
  }
}

green_t *findNextReady() {
  if(first == NULL && last == NULL) {
    printf("Failed, no threads in ready queue");
  }
  else {
    green_t *ret;
    ret = first;

    if(ret->next != NULL) {
      first = ret->next;
      printf("next is null\n");
    }
    else {
      last = ret;
      first = ret;
    }

    return ret;
  }
}

void green_thread() {
  green_t *this = running;

  // place waiting (joining) thread in ready queue
   if(this->join != NULL) {
     addLast(this->join);
   }

  // save result of execution
   this->retval = (*this->fun)(this->arg);
   printf("Zombie: %d\n", this->zombie);

  // free allocated memory structures
  printf("Running free for thread: %p\n", this);
   free(this->context->uc_stack.ss_sp);
   free(this->context);
  printf("free succeeded\n");
  printf("Zombie: %d\n", this->zombie);
  // we're a zombie
   this->zombie = TRUE;
   printf("Setting zombie");

  // find the next thread to run
   green_t *next = findNextReady();
   printf("Setting next");
   running = next;
   setcontext(next->context);
}

int green_create(green_t *new, void *(*fun)(void*), void *arg) {

  ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
  getcontext(cntx);

  void *stack = malloc(STACK_SIZE);

  cntx->uc_stack.ss_sp = stack;
  cntx->uc_stack.ss_size = STACK_SIZE;
  makecontext(cntx, green_thread, 0);

  new->context = cntx;
  new->fun = fun;
  new->arg = arg;
  new->next = NULL;
  new->join = NULL;
  new->retval = NULL;
  new->zombie = FALSE;

  // add new to the ready queue
  addLast(new);

  return 0;
}

int green_yield() {
  green_t *susp = running;
  // add susp to ready queue
  addLast(susp);
  // select the next thread for execution
  green_t *next = findNextReady();
  running = next;
  swapcontext(susp->context, next->context);
  return 0;
}

int green_join(green_t *thread, void **res) {
  if(thread->zombie) {
    res = thread->retval;
    return 0;
  }
  green_t *susp = running;
  // add as joining thread
  thread->join = susp;

  //select the next thread for execution
  green_t *next = findNextReady();

  running = next;
  swapcontext(susp->context, next->context);
  return 0;
}
