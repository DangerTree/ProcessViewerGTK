/* Sarah Gunderson. CSCI 352 UNIX. Spring 2016. Dr Bover.

    testProcAccess.c is the driver function for the process monitor
        it depends on gtkProcStruct.c

    HOW:
-A pidNode contains all process info for a process
-pidArray is a static array of pidStructs created to hold current pid info
    
    ~In update, the program reads each numerical directory in /proc and adds all info to a pidNode and puts the node in the pidList.
    ~The Node is then added to the list store (LS) to be displayed
    ~update is called every second
    ~Next time, if pid is already in list store, update cpu % and mem vals and delete dead processes whose pid < thisPid
    ~If pid is not in LS, create a new pidNode, add it to the pidArray, and delete dead processes whose pid < thisPid 
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gtk/gtktreemodelsort.h>
#include <stdbool.h>
#include "gtkProcStruct.h"

#define MAX_FILE 1024
#define MAX_PID 32768
#define MAX_WORD 64

static int update (GtkListStore *);

// establish a stucture in which to hold process information
typedef struct pidNode{
    gchar * proc_name;
    gchar * user_name;
    gfloat cpu_percent;
    gint pid;
    gfloat mem_using;
    int oldProcTicks;
    int oldCpuTicks;
} pidNode;

// establish a static array with a slot for every possible pid
static pidNode * pidArray [MAX_PID];

static GtkListStore * store;


/* getCPUticks calculate the total CPU time spent     
 it does this by adding the following info from /proc/stat:
 user, nice, system, irq, softirq
*/ 
int getCPUticks (){
    
    int totalCPUtime = 0;

    // open a file stream to /proc/stat
    FILE * cpuProc = fopen ("/proc/stat", "r");
    char cpuTickLine [MAX_FILE];

    // read all lines from /proc/stat into cpuTickLine
    while (fgets (cpuTickLine, MAX_FILE, cpuProc) != NULL){
        if (cpuTickLine [0] == 'c'){
            
            // get the user, nice, system, irq, and softirq ticks
            char * lineStart;
            lineStart = cpuTickLine + 4;
            char * end = lineStart;
            for (int i = 0; i < 7; i++){
                int n = strtol (lineStart, &end, 10);
                if (i == 0 || i == 1 || i == 2 || i == 5 || i == 6){
                    totalCPUtime += n;
                }
                lineStart = end;
            }
        }
        else { break; }
    }
    fclose (cpuProc);
    
    return totalCPUtime;
}


// calcProcTicks returns the sum of utime and stime of one proc
int calcProcTicks(char * pidPath){
    
    // open stat file
    char statPath [MAX_WORD];
    strncat (strncpy (statPath, pidPath, strlen(pidPath)+1), "/stat", 6);
    char statLine [MAX_FILE];
    int newProcTicks = 0;

    // get sum of utime and stime of process from /proc/[pid]/stat
    FILE * pidStat = fopen (statPath, "r");
    if (pidStat == NULL){
        return 0;
    }       
    
    if (fgets (statLine, MAX_FILE, pidStat) != NULL){
        // go to 14th string in stat, utime
        char * lineStart = statLine;
        char * end = lineStart;
        // move lineStart to point to #3 position in statLine (after chars)
        char * junk1 = malloc (MAX_WORD);
        char * junk2 = malloc (MAX_WORD);
        char * junk3 = malloc (MAX_WORD);
        sscanf (end, "%s%s%s", junk1, junk2, junk3);
        int offset = 3 + strlen(junk1) + strlen(junk2) + strlen (junk3);
        free (junk1);
        free (junk2);
        free (junk3);

        // offset to the third token after the chars
        lineStart += offset;
        
        // for the next 12 things in stat, add the 14th and 15th strings (utime, stime) to total time
        for (int i = 3; i < 15; i++){
            int n = strtol (lineStart, &end, 10);
            if (i == 13 || i == 14){
                newProcTicks += n;
            }
            lineStart = end;
        }
    }
    fclose (pidStat);
    return newProcTicks;
}


// calcCPU calculates the %CPU and puts the correct value into the pidNode
// int isNew is 1 if this is a new process, and 0 is if is old
void calcCPU (pidNode * pNode, char * pidPath, int isNew){

    // calculate current number of ticks for the proc and CPU 
    int newProcTicks = calcProcTicks (pidPath);
    int newCpuTicks = getCPUticks();
    
    // if this is a new process, fill possible fields and return
    // OR if process has suddenly died
    if (isNew == 1 || newProcTicks == 0){
        pNode->oldProcTicks = newProcTicks;
        pNode->oldCpuTicks =  newCpuTicks;
        pNode->cpu_percent = 0.0;
        return;
    }
    
    // otherwise, this process is already in pidArray.
    float cpuPrcnt = ((float) newProcTicks - (float) pNode->oldProcTicks)/((float) newCpuTicks - (float) pNode->oldCpuTicks);
    
    cpuPrcnt = cpuPrcnt * 100;    

    char retStr [20];
    snprintf (retStr, 20, "%.2f", cpuPrcnt); 
    gfloat toRet = (gfloat) atof (retStr);
    pNode->cpu_percent = toRet;
    // update pNode tick vals
    pNode->oldCpuTicks = newCpuTicks;
    pNode->oldProcTicks = newProcTicks;
}


// getProcName returns the Process Name of a pid
gchar * getProcName (pidNode * pNode, char * pidPath){

    // set procCommPath = proc/[pid]/comm    
    int pathLen = strlen (pidPath) + 6;
    char * procName = malloc (MAX_WORD);
    char procCommPath [pathLen];
    strncat (strncpy (procCommPath, pidPath, strlen(pidPath)+1), "/comm", 5);

    FILE * pid = fopen (procCommPath, "r");
    if (pid == NULL){
        return (gchar *) "Dead";
    }
    fgets (procName, pathLen, pid);
    fclose (pid);
    //printf("\t%s", procName);  FOR TESTING
    
    int pNameSize = strlen(procName) + 1;
    char * pNameStr = malloc (pNameSize);
    strncpy (pNameStr, procName, pNameSize);
    return (gchar *) pNameStr;
}


// getMem returns the gfloat size of the program in MB from a pid's statm
gfloat getMem (char * pidPath){
    
    char procStatm [MAX_WORD];
    char memStrRaw [MAX_WORD];
    
    // assign procStatm to /proc/[pid]/statm
    strncat (strncpy (procStatm, pidPath, strlen(pidPath)+1), "/statm", 7);
    
    FILE * memFile = fopen (procStatm, "r");

    // in the unlikely chance that the proc ended since getMem was called
    if (memFile == NULL){
        return (gfloat) 0.0;
    }

    // get total page # for process
    fgets (memStrRaw, MAX_WORD, memFile);
    gfloat memUsed = atoi (strtok (memStrRaw, " "));
    fclose (memFile);
    
    // printf ("\t%f", memUsed);  FOR TESTING 

    memUsed = memUsed * 4 / 1024; // get size in MB

    return (gfloat) memUsed;
}


// getUserName returns a string rep of the User / Owner of the process from pidPath
gchar * getUserName (char * pidPath){

    struct stat * fileStat = malloc (sizeof (struct stat));
    struct passwd * pwd;
    
    // get stat info on the file
    stat (pidPath, fileStat);            
    // and print out the user owning the process
    pwd = getpwuid(fileStat->st_uid);
    
    int usrNameSize = strlen (pwd->pw_name);
    char * user = malloc (usrNameSize + 1);    
    char holder [usrNameSize+1];
    strncpy (holder, pwd->pw_name, usrNameSize);
    holder [usrNameSize] = '\0';
    strncpy (user, holder, usrNameSize+1);
    // printf("\t%s", user);   FOR TESTING
    
    free (fileStat);
    return (gchar *) user;
}


// free pidArray of dead processes and updates LS iterator
void refreshListStore (GtkListStore * store, GtkTreeIter * iter, int pidNum){ 
    int ctr = 0;
    gint LSpid;

    // while the iter is not to the end of the LS
    while (iter != FALSE || iter != 0){
        // and the listStore iter-> pid != this pid
        gtk_tree_model_get (GTK_TREE_MODEL (store), iter, PID, &LSpid, -1);
        LSpid = (int) LSpid;
        // delete the row from the listStore and pidList
        if (LSpid < pidNum){
            free (pidArray [LSpid]->proc_name);
            free (pidArray [LSpid]->user_name);
            pidArray [LSpid] = NULL;
            gtk_list_store_remove (GTK_LIST_STORE (store), iter);
            ctr ++;
        }
        else { break; }
    }
}


// addNewRow sets the list store with data, incl an image dep. on user_name
void addNewRow (pidNode * newNode, GtkTreeIter * iter, int updateCntr){
    // add the process info to new row in liststore
    //gtk_list_store_append (store, &iter);
    GError *error = NULL;
    GdkPixbuf * image;
    if (strcmp (newNode->user_name , "root") == 0){
        image = gdk_pixbuf_new_from_file ("paletaVerde.png", &error);
    }
    else {
        image = gdk_pixbuf_new_from_file ("paletaRoja.png", &error);
    }


        gtk_list_store_insert_with_values (store, iter, updateCntr,
                        PROC_NAME, newNode->proc_name, 
                        IMAGE, image,
                        USER_NAME, newNode->user_name,
                        CPU_PERCENT, newNode->cpu_percent,
                        PID, newNode->pid,
                        MEM_USING, newNode->mem_using, -1);

    
}


/* update: goes through all processes and puts them + info in liststore
*/
int update (GtkListStore * store){
    
    gchar * pid;

    // establish listStore iter to start of listStore
    GtkTreeIter * iter = malloc (sizeof (GtkTreeIter));
    gboolean valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), iter);

    // establish a DIR and stru
    DIR * procDir;
    struct dirent * procDirent;
    char path [MAX_WORD] = "/proc/";
    char pidPath [MAX_WORD] = "/proc/";

    procDir = opendir (path);
    int updateCntr = -1;

    // for all entries in the directory that start with a number
    while ((procDirent = readdir (procDir)) != NULL){
        pid = procDirent->d_name;
        if (pid[0] >= 48 && pid[0] <= 57){
            // printf("%s\t", pid);   FOR TESTING 
            int pidNum = atoi (pid); // store pid

            updateCntr++;

            // store /proc/[pid] path
            strncat (pidPath, pid, strlen(pid));
            // printf("%s\t", pidPath);   FOR TESTING
            
            // UPDATE PIDARRAY AND LIST STORE
            // if pid is in the pidArray, update memUsed and cpu% in [] + LS
            if (pidArray [pidNum] != NULL){
                pidArray [pidNum] -> mem_using = getMem (pidPath);
                calcCPU (pidArray [pidNum], pidPath, 0);
                
                // remove old processes, set iter to correct row
                refreshListStore (store, iter, pidNum);

                // update the listStore with pidList vals
                gtk_list_store_set (store, iter, MEM_USING, pidArray[pidNum]->mem_using, CPU_PERCENT, pidArray[pidNum]->cpu_percent, -1);
                
                // set iterator to next row
                valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), iter);
                // printf("\n");   FOR TESTING
            }

            // if pid is not in the pidArray
            else {
                // add all vals to the node
                pidNode * newNode = malloc (sizeof (pidNode));
                newNode->mem_using = getMem (pidPath);
                newNode->user_name = getUserName (pidPath);
                newNode->pid = (gint) pidNum;
                newNode->proc_name = getProcName (newNode, pidPath);
                calcCPU (newNode, pidPath, 1);

                // add the pid to the pidList
                pidArray [newNode->pid] = newNode;
                
                // update iter to correct place in LS, del dead procs
                if (valid){
                    refreshListStore (store, iter, pidNum);
                }
                addNewRow (newNode, iter, updateCntr);
                valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), iter);
            }

            // reset path to "/proc/" by appending a null char
            pidPath[6] = '\0';}
    }
    closedir(procDir);
    return 1;
}


// setSorts makes all cols sortable by setting sorting funcs
void setSorts (GtkTreeModel * sortmodel){
    
    // make process names sortable
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortmodel),
                            PROC_NAME, sort_alphabatize,
                            GINT_TO_POINTER (PROC_NAME), NULL);
    
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sortmodel), 
                            PROC_NAME,  GTK_SORT_ASCENDING);

    // make user names sortable
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortmodel),
                            USER_NAME, sort_alphabatize,
                            GINT_TO_POINTER (PROC_NAME), NULL);
    
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sortmodel), 
                            USER_NAME,  GTK_SORT_ASCENDING);

    // make cpu % sortable
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortmodel),
                            CPU_PERCENT, sort_numbers,
                            GINT_TO_POINTER (CPU_PERCENT), NULL);
    
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sortmodel), 
                            CPU_PERCENT,  GTK_SORT_DESCENDING);

    // make PID sortable
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortmodel),
                            PID, sort_numbers,
                            GINT_TO_POINTER (PID), NULL);
    
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sortmodel), 
                            PID,  GTK_SORT_ASCENDING);

    // make Mem sortable
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (sortmodel),
                            MEM_USING, sort_numbers,
                            GINT_TO_POINTER (MEM_USING), NULL);
    
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sortmodel), 
                            MEM_USING,  GTK_SORT_ASCENDING);
}


int main (){

    gtk_init (0, NULL);

    store = gtk_list_store_new (COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_INT, G_TYPE_FLOAT);

    // sortmodel can be sorted in the tree view without messing order of LS
    GtkTreeModel * sortmodel;
    sortmodel = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (store));

    // add values to LS
    update (store);
    
    // create timeout refresh
    g_timeout_add_seconds (1, (GSourceFunc) update, (gpointer) store);
    
    // create treeview using the sortable list store
    GtkWidget * treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (sortmodel));

    // set up the column
    build_treeview (treeview);

    // unreference the model so it will be destroyed when window closes
    g_object_unref (store);

    // display the tree
    display (treeview);

    gtk_main ();

    return 1;
}



