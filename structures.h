#define STRUCTURES_H_

// defining structures
struct type_r {
    int rd;
    char func3;
    int rs1;
    int rs2;
    char func7;
};
struct type_i {
    int rd;
    char func3;
    int rs1;
    int imm;
};
struct type_s {
    char func3;
    int rs1;
    int rs2;
    int imm;
};
struct type_sb {
    char func3;
    int rs1;
    int rs2;
    int imm;
};
struct type_u {
    int rd;
    int imm;
};
struct type_uj {
    int rd;
    int imm;
};

// how registers will be
union char_to_int {
    char chars[4];
    int imm;
}; 
// allocating register virtual memory
struct registers {
    union char_to_int values;
};

struct linked_list{
    int start;
    int len;
    struct linked_list * next;
};

typedef struct linked_list * node;


