// file tests proc data access

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <string.h>
#include <gtk/gtk.h>
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
    //gchar * mem_using;
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
    char statLine [MAX_FILE]; // this is way too big
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

        lineStart += offset;
        
        // for the first 15 things in stat, add the 14th and 15th strings (utime, stime) to total time
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
//char * getProcName (char * pidPath){
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
    printf("\t%s", procName);
    
    int pNameSize = strlen(procName) + 1;
    char * pNameStr = malloc (pNameSize);
    strncpy (pNameStr, procName, pNameSize);
    return (gchar *) pNameStr;
}


// getMem returns the size of the program from a pid's statm, and converts to MB
gfloat getMem (char * pidPath){
    
    char procStatm [MAX_WORD];
    char memStrRaw [MAX_WORD];
    
    // assign procStatm to /proc/[pid]/statm
    strncat (strncpy (procStatm, pidPath, strlen(pidPath)+1), "/statm", 7);
    
    FILE * memFile = fopen (procStatm, "r");

    // in the unlikely chance that the proc ended since getMem was called
    if (memFile == NULL){
        //gchar * toRet = "N/A\0";
        //return toRet;
        return (gfloat) 0.0;
    }

    fgets (memStrRaw, MAX_WORD, memFile);
    gfloat memUsed = atoi (strtok (memStrRaw, " "));
    fclose (memFile);
    
    printf ("\t%f", memUsed);           

    memUsed = memUsed * 4 / 1024; // get size in MB

    return (gfloat) memUsed;

    // charMemUsed is char rep of int with two decimal places
    /*char * memStrDone = malloc (16);
    char * charMemUsed = malloc (16);
    sprintf (charMemUsed, "%.2f", memUsed);
    if (memUsed != 0){
        if (strlen(charMemUsed) < 7){ // KB
            //memUsed = memUsed;
            sprintf (memStrDone, "%.2f", memUsed);
            strncat (memStrDone, " KB\0", 4);
        }
        else {
            memUsed = memUsed / 1024;
            sprintf (memStrDone, "%.2f", memUsed);
            strncat (memStrDone, " MB\0", 4);
        }
    }
    else {
        memStrDone = "N/A\0";
    }*/
    //return (gchar *) memStrDone;
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
    printf("\t%s", user);
    
    free (fileStat);
    return (gchar *) user;
}


/* addNewLSRow creates a new row for data in a list store,
and populates it with data from a pidNode
*/
void addNewLSRow (pidNode * newNode, GtkTreeIter * iter, GtkListStore * store){

    // create a new row in listStore
    gtk_list_store_append (store, iter);

    // add the pidNode to the listStore
    gtk_list_store_set (store, iter, PROC_NAME, newNode->proc_name, 
                                    USER_NAME, newNode->user_name,
                                    CPU_PERCENT, newNode->cpu_percent,
                                    PID, newNode->pid,
                                    MEM_USING, newNode->mem_using, -1);
}


// free pidArray of dead processes and updates LS iterator
void refreshListStore (GtkListStore * store, GtkTreeIter * iter, int pidNum){ 
    int ctr = 0;
    //gboolean valid = TRUE;    
    gint LSpid;

    // while the iter is not to the end of the LS
    while (iter != FALSE || iter != 0){
        // and the listStore iter-> pid != this pid
        gtk_tree_model_get (GTK_TREE_MODEL (store), iter, PID, &LSpid, -1);
        LSpid = (int) LSpid;
        // delete the row from the listStore
        if (LSpid < pidNum){
            // delete the that pid from the pidList
            pidArray [LSpid] = NULL;
            gtk_list_store_remove (GTK_LIST_STORE (store), iter);
            ctr ++;
            // if iter is not pointing to 
        }
        else { break; }
    }
    //return ctr;
}


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
            printf("%s\t", pid);            
            int pidNum = atoi (pid); // store pid

            updateCntr++;

            // store /proc/[pid] path
            strncat (pidPath, pid, strlen(pid));
            printf("%s\t", pidPath);
            
            // UPDATE PIDARRAY AND LIST STORE
            // if pid is in the pidArray, update memUsed and cpu%
            if (pidArray [pidNum] != NULL){
                pidArray [pidNum] -> mem_using = getMem (pidPath);//memUsed;
                calcCPU (pidArray [pidNum], pidPath, 0);
                
                // remove old processes, set iter to correct row
                refreshListStore (store, iter, pidNum);

                // update the listStore with pidList vals
                gtk_list_store_set (store, iter, MEM_USING, pidArray[pidNum]->mem_using, CPU_PERCENT, pidArray[pidNum]->cpu_percent, -1);
                
                // set iterator to next row
                valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), iter);
                // if iter is at end of LS, append a row in case there is more pids to store
/*                if (nextIter == FALSE){
                    gtk_list_store_append (store, &iter);
                }
*/                printf("\n");
            }

            // if pid is not in the pidArray
            else {
                // add all vals to the node
                pidNode * newNode = malloc (sizeof (pidNode));
                newNode->mem_using = getMem (pidPath);
                newNode->user_name = getUserName (pidPath);
                newNode->pid = (gint) pidNum;
                newNode->proc_name = getProcName (newNode, pidPath);
                //newNode->image = getImage (char * userName);
                calcCPU (newNode, pidPath, 1);

                // add the pid to the pidList
                pidArray [newNode->pid] = newNode;
                
                // update iter to correct place in LS, del dead procs
                if (valid){
                    refreshListStore (store, iter, pidNum);
                }
                
                addNewRow (newNode, iter, updateCntr);

                // add the process info to new row in liststore
                //gtk_list_store_append (store, &iter);
/*                gtk_list_store_insert_with_values (
                                    store, iter, updateCntr,
                                    PROC_NAME, newNode->proc_name, 
                                    USER_NAME, newNode->user_name,
                                    CPU_PERCENT, newNode->cpu_percent,
                                    PID, newNode->pid,
                                    MEM_USING, newNode->mem_using, -1);
  */              
                valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), iter);
                // add the pidNode to the listStore
/*                gtk_list_store_set (store, &iter, PROC_NAME, newNode->proc_name, USER_NAME, newNode->user_name, CPU_PERCENT, newNode->cpu_percent, PID, newNode->pid, MEM_USING, newNode->mem_using, -1);
                addNewLSRow (newNode, iter, store);
*/            }

            // reset path to "/proc/" by appending a null char
            pidPath[6] = '\0';}
    }
    closedir(procDir);
    return 1;
}



int main (){

    gtk_init (0, NULL);

    store = gtk_list_store_new (COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_INT, G_TYPE_FLOAT);

    // initiate sorting
    // ---------------------
    GtkTreeSortable * sortable = GTK_TREE_SORTABLE (store);

    gtk_tree_sortable_set_sort_func (sortable, PROC_NAME, sort_proc_names, GINT_TO_POINTER (PROC_NAME), NULL);
    
    gtk_tree_sortable_set_sort_column_id (sortable, PROC_NAME, 
                                        GTK_SORT_ASCENDING);
    
    update (store);
    
    // create treeview (columns)
    GtkWidget *treeview = gtk_tree_view_new ();
    build_treeview (treeview);

    // create timeout refresh
    g_timeout_add_seconds (1, (GSourceFunc) update, (gpointer) store);
    
    // add the model to the tree view
    gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (store));

    // unreference the model so it will be destroyed when window closes
    g_object_unref (store);

    // display the tree
    display (treeview);

    gtk_main ();

    return 1;
}



