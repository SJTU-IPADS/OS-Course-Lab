/* A list node whose four fields have different sizes: the exercise of the
   part on arrays and structs.

     gcc -Og -fcf-protection=none -S node.c -o - | sed -f asm.sed */
struct Node {
    char *name;
    int val[3];
    struct Node *next;
    char flags;
};

unsigned long size(void) {
    return sizeof(struct Node);
}

int val_at(struct Node *p, long i) {
    return p->val[i];
}

int name_at(struct Node *p, long i) {
    return p->name[i];
}

void set_val(struct Node *p, long i) {
    p->next->val[i] = p->flags;
}
