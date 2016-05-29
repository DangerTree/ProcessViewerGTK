#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gtkProcStruct.h"

#define MAX_LINE 100
#define MAX_PID 32768


void mem_float_cell_display (GtkTreeViewColumn* col, 
                       GtkCellRenderer* renderer,
                       GtkTreeModel* model,
                       GtkTreeIter* iter,
                       gpointer colNo)  {
    gfloat theFloat;
    gchar buf[20];
    
    gtk_tree_model_get (model, iter, colNo, &theFloat, -1);
    
    // do calc here to append MiB or KiB
    g_snprintf (buf, 20, "%.2f", theFloat);

    
    g_object_set (renderer, "text", buf, NULL);
}


// A helper function for build_treeview. Makes a long int have 2 places right of decimal. If i == 0, this will put the data in % mode. i ==1 will calc size.
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
    
    //****************************************************added for sorting
    // set the sort column and indicator
    gtk_tree_view_column_set_sort_column_id(column, PROC_NAME);
    gtk_tree_view_column_set_sort_indicator(column, TRUE);
    //***************************************************
    
    // add the user name to the treeview
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes (
                "User", renderer, "text", USER_NAME,
                NULL); 
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
    
    
     // add the CPU % to the treeview
    renderer = gtk_cell_renderer_text_new();
/*    column = gtk_tree_view_column_new_with_attributes ("\% CPU", renderer, "text", CPU_PERCENT, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
*/
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, "\% CPU");
    gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, float_cell_display, GINT_TO_POINTER(CPU_PERCENT) , NULL);

    // add PID to the treeview
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes ("ID", renderer, "text", PID, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    // add Memory Usage to treeview
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes (
                "Memory", renderer, "text", MEM_USING,
                NULL); 
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
    /*column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, "Memory");
    gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    
    gtk_tree_view_column_set_cell_data_func (column, renderer, float_cell_display, GINT_TO_POINTER(MEM_USING) , NULL);
   */ 
}

void display (GtkWidget *treeview) {
    
    // create the window
    GtkWidget* window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Processes Running");
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (window, 400, 400);
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

//****************************************************added for sorting
// define sort order on names
gint sort_proc_names (GtkTreeModel *model,
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
//*************************************************************************

/*
int main (int argc, char* argv[]) {

    
    // expect the input file name on the command line
    if (argc < 2) {
        printf("Usage: listdemo infile\n");
        return 1;
    }
    
    gtk_init (&argc, &argv);
        
    
    gtk_init (0, NULL);

    // build the list store from the file data
    GtkListStore *store = gtk_list_store_new (COLUMNS, G_TYPE_STRING,
                                              G_TYPE_STRING, G_TYPE_STRING,
                                              G_TYPE_INT, G_TYPE_FLOAT, 
                                              GDK_TYPE_PIXBUF);

    ****************************************************added for sorting
    // make the treeview sortable on all categories
    GtkTreeSortable *sortable = GTK_TREE_SORTABLE (store);
    
    gtk_tree_sortable_set_sort_func (sortable, PROC_NAME, sort_proc_names, 
                                     GINT_TO_POINTER (PROC_NAME), NULL);
    
    gtk_tree_sortable_set_sort_column_id (sortable, PROC_NAME,
                                          GTK_SORT_ASCENDING);
    ****************************************************************

    // create the tree view of the list (all the columns)
    GtkWidget *treeview = gtk_tree_view_new ();
    build_treeview(treeview);


    // create timeout refresh
    // g_timeout_add_seconds(1, (GSourceFunc) build_list, (gpointer) data);

    if (build_list(argv[1], store) != 0) {
        printf("Error building list from data\n");
        return 1;
    }
    
    // add the tree model to the tree view
    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));
    
    // unreference the model so that it will be destroyed when the tree
    // view is destroyed
    g_object_unref (store);
    
    // display the tree view
    display (treeview);
 
    gtk_main ();

    return 0;
}
*/
