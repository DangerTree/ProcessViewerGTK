/* gtkProcStruct.c sets up windows for the process manager.

    sets up the columns
    creates sorting functions
    displays the window
*/

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gtkProcStruct.h"

#define MAX_LINE 100
#define MAX_PID 32768


// A helper function for build_treeview. Makes a long int have 2 places right of decimal.
void mem_float_cell_display (GtkTreeViewColumn* col, 
                       GtkCellRenderer* renderer,
                       GtkTreeModel* model,
                       GtkTreeIter* iter,
                       gpointer colNo)  {
    gfloat theFloat;
    gchar buf[20];
    
    gtk_tree_model_get (model, iter, colNo, &theFloat, -1);
    g_snprintf (buf, 20, "%.2f", theFloat);
    if (theFloat != 0.0){
        strncat (buf, " MB\0", 4);
        //int sLen = strlen (buf);
        //buf [sLen] = "  MB\0";
    }
    g_object_set (renderer, "text", buf, NULL);
}


// A helper function for build_treeview. Makes a long int have 2 places right of decimal.
void float_cell_display (GtkTreeViewColumn* col, 
                       GtkCellRenderer* renderer,
                       GtkTreeModel* model,
                       GtkTreeIter* iter,
                       gpointer colNo)  {
    gfloat theFloat;
    gchar buf[20];
    
    gtk_tree_model_get (model, iter, colNo, &theFloat, -1);
    g_snprintf (buf, 20, "%.2f", theFloat);
    g_object_set (renderer, "text", buf, NULL);
}


// build_treeview sets up the columns for the widget and tells them how to sort
void build_treeview (GtkWidget *treeview) {
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    // add the process name to the treeview, with an icon
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "pixbuf", IMAGE, NULL);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", PROC_NAME,
                NULL); 
    
    gtk_tree_view_column_set_title(column, "Process Name");
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
    
    // tell the image / proc name column how to sort
    gtk_tree_view_column_set_sort_column_id(column, PROC_NAME);
    gtk_tree_view_column_set_sort_indicator(column, TRUE);
    //***************************************************
    
    // add the user name to the treeview
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes (
                "User", renderer, "text", USER_NAME, NULL); 
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
    
    // tell the user name column how to sort
    gtk_tree_view_column_set_sort_column_id(column, USER_NAME);
    gtk_tree_view_column_set_sort_indicator(column, TRUE);
    //***************************************************
    
    // add the CPU % to the treeview
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, "\% CPU");
    gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, float_cell_display, GINT_TO_POINTER(CPU_PERCENT) , NULL);

    // tell the CPU % column how to sort
    gtk_tree_view_column_set_sort_column_id(column, CPU_PERCENT);
    gtk_tree_view_column_set_sort_indicator(column, TRUE);
    //***************************************************

    // add PID to the treeview
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes ("ID", renderer, "text", PID, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    // tell the PID column how to sort
    gtk_tree_view_column_set_sort_column_id (column, PID);
    gtk_tree_view_column_set_sort_indicator (column, TRUE);
    //***************************************************
    
    // add Memory Usage to treeview
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, "Memory");
    gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, mem_float_cell_display, GINT_TO_POINTER(MEM_USING) , NULL);
    
    // tell the mem used column how to sort
    gtk_tree_view_column_set_sort_column_id(column, MEM_USING);
    gtk_tree_view_column_set_sort_indicator(column, TRUE);
}


// display does the dirty work to create / display the window
void display (GtkWidget *treeview) {
    
    // create the window
    GtkWidget* window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Processes Running");
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (window, 450, 400);
    g_signal_connect (window, "delete_event", gtk_main_quit, NULL);
    
    // create a scrolled window
    GtkWidget* scroller = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    
    // pack the containers
    gtk_container_add (GTK_CONTAINER (scroller), treeview);
    gtk_container_add (GTK_CONTAINER (window), scroller);
    gtk_widget_show_all (window);
}


//******************************************added for sorting
// define sort order on strings
gint sort_alphabatize (GtkTreeModel *model,
                      GtkTreeIter *a, GtkTreeIter *b,
                      gpointer data) {
    
    gchar *name1, *name2;
    gtk_tree_model_get (model, a, PROC_NAME, &name1, -1);
    gtk_tree_model_get (model, b, PROC_NAME, &name2, -1);
    
    int order = strcmp(name1, name2);
    g_free(name1);
    g_free(name2);
    
    return -order;
}

// define sort order on numbers
gint sort_numbers (GtkTreeModel * model, GtkTreeIter * a,
                    GtkTreeIter * b, gpointer data){
    gfloat num1;
    gfloat num2;
    gtk_tree_model_get (model, a, CPU_PERCENT, &num1, -1);
    gtk_tree_model_get (model, a, CPU_PERCENT, &num2, -1);

    int result = num1 - num2;   
 
    return result; 
}
