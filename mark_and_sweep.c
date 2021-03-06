#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(cond, msg, ...) ((cond) ? (int)0x1337 : (printf("\033[1;31m[ASSERT]\033[0m " msg "\n", __VA_ARGS__)))
#define LOG(type, msg, ...) (printf("\033[1;32m[" type " LOG]\033[0m " msg "\n", __VA_ARGS__))

typedef enum {
  INT_TYPE,
  STRING_TYPE,
  OBJ_TYPE
} TYPE;

typedef struct mObject {
  TYPE type;
  unsigned char is_marked;
  struct mObject *next;
  union {
    // primitive type for INT_TYPE
    int primitive_value;

    // primitive type for STRING_TYPE
    char *string;

    // linked list for OBJ_TYPE
    struct {
      struct mObject *head;
      struct mObject *tail;
    };
  };
} Object;

struct file_operations;

#define MAX 0x100
// This vm is stack based machine
typedef struct {
  Object *stack[MAX];
  Object *root;
  unsigned int size;
  unsigned int numObjs;
  unsigned int maxObjs;

  // file operations
  // Need to be implemented in OOP Model...
  struct file_operations *fops;
} simpleVM;

// push
void push(simpleVM *vm, Object *value) {
  if (ASSERT(vm->size < MAX, "Stack size is overflowed, [current : %d]", vm->size) != 0x1337) {
    exit(-1);
  }
  vm->stack[vm->size++] = value;
}

// pop
Object *pop(simpleVM *vm) {
  if (ASSERT(vm->size > 0, "%s", "Stack size is underflowed") != 0x1337) {
    exit(-1);
  }
  return vm->stack[--vm->size];
}

// Forward declare
void gc(simpleVM *vm);
Object *createObject(simpleVM *vm, TYPE type) {
  // Need to collect garbage
  if (vm->numObjs == vm->maxObjs) {
    gc(vm);
  }

  Object *object = (Object *)malloc(sizeof(Object));
  object->type = type;

  // set linked list
  object->next = vm->root;
  vm->root = object;

  object->is_marked = 0;
  vm->numObjs++;
  return object;
}

void pushInt(simpleVM *vm, int value) {
  Object *object = createObject(vm, INT_TYPE);
  object->primitive_value = value;
  push(vm, object);
  LOG("pushInt", "push Int : %d", value);
}

void pushString(simpleVM *vm, char *string) {
  Object *object = createObject(vm, STRING_TYPE);
  object->string = string;
  push(vm, object);
  LOG("pushString", "push String : %s", string);
}

// Pair Object :)
Object *pushObject(simpleVM *vm) {
  Object *object = createObject(vm, OBJ_TYPE);
  object->tail = pop(vm);
  object->head = pop(vm);

  push(vm, object);
  LOG("pushObject", "push Object pair : %p", object);
  return object;
}

void dump(Object *object) {
  switch (object->type) {
    case INT_TYPE:
      LOG("DUMP", "INT_TYPE : %d", object->primitive_value);
      break;

    case STRING_TYPE:
      LOG("DUMP", "STRING_TYPE : %s", object->string);
      break;

    case OBJ_TYPE:
      LOG("DUMP", "%s", "Will be implemented");
      break;

    default:
      LOG("DUMP", "%s", "UNKNOWN");
      break;
  }
}

struct file_operations {
  void (*pushInt)(simpleVM *, int);
  void (*pushString)(simpleVM *, char *);
  Object *(*pushObject)(simpleVM *);
  Object *(*pop)(simpleVM *);
};

simpleVM *createVM() {
  simpleVM *vm = (simpleVM *)malloc(sizeof(simpleVM));
  vm->size = 0;
  vm->numObjs = 0;
  vm->maxObjs = 0;
  vm->root = NULL;

  vm->fops = (struct file_operations *)malloc(sizeof(struct file_operations));
  vm->fops->pushInt = pushInt;
  vm->fops->pushString = pushString;
  vm->fops->pushObject = pushObject;
  vm->fops->pop = pop;

  LOG("createVM", "VM is created : %p", vm);
  return vm;
}

void mark(Object *object) {
  if (object->is_marked) {
    return;
  }

  object->is_marked = 1;
  if (object->type == OBJ_TYPE) {
    mark(object->head);
    mark(object->tail);
  }
}

void mark_phase(simpleVM *vm) {
  for (int i = 0; i < vm->size; i++) {
    mark(vm->stack[i]);
  }
}

void sweep(simpleVM *vm) {
  Object *object = vm->root;
  while (object) {
    // reached -> doesn't need to GC
    if (object->is_marked) {
      // For next GC
      object->is_marked = 0;
      object = object->next;
    }
    // unreached
    else {
      Object *unreached = object;
      object = unreached->next;
      free(unreached);
      vm->numObjs--;
    }
  }
}

void gc(simpleVM *vm) {
  unsigned int numObjs = vm->numObjs;

  mark_phase(vm);
  sweep(vm);

  // Expand storage
  vm->maxObjs = vm->numObjs * 2;
  LOG("GC", "Collected %d objects, %d remaining.", numObjs - vm->numObjs, vm->numObjs);
}

int main(int argc, char *argv[]) {
  LOG("MAIN", "%s", "Garbage Collector (Mark and Sweep Algorithm Test");
  simpleVM *vm = createVM();
  for (int i = 0; i < 0x10; i++) {
    vm->fops->pushInt(vm, 0x41 + i);
  }

  vm->fops->pushString(vm, "asiagaming");

  Object *object = vm->root;
  while (object) {
    dump(object);
    object = object->next;
  }

  // Primitive value isn't needed to be GC
  Object *t1 = vm->fops->pushObject(vm);
  Object *t2 = vm->fops->pushObject(vm);
  t1->tail = t2;
  t2->tail = t1;

  // Collect 1
  gc(vm);
  return 0;
}
