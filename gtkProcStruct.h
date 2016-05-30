// gtkProcStruct.h is header file to be ref'd by testProcAccess.c

#ifndef MYPROC_H
#define MYPROC_H

enum {
    IMAGE,
    PROC_NAME,
    USER_NAME,
    CPU_PERCENT,
    PID,
    MEM_USING,
    COLUMNS
};

void float_cell_display (GtkTreeViewColumn * col, GtkCellRenderer * renderer, GtkTreeModel * model, GtkTreeIter * iter, gpointer colNo);

void build_treeview (GtkWidget * treeview);

void display (GtkWidget * treeview);

gint sort_alphabatize (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter *b, gpointer data);

gint sort_numbers (GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer data);

#endif


