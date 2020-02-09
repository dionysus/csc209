#include <stdio.h>
// Add your system includes here.
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include "ftree.h"

// HELPER FUNCTIONS
    // make nodes
    struct TreeNode *make_node (const char *fname, char *path);
    struct TreeNode *node_maker(struct stat stat_buf, const char *fname, char *filepath);
    // open directories + populate nodes
    struct TreeNode *make_children(const char *fname, char *filepath);
    struct TreeNode *children_maker(struct dirent *entry_ptr, DIR *d_ptr, const char *fname, char *path);

/*
 * Returns the FTree rooted at the path fname.
 *
 * Use the following if the file fname doesn't exist and return NULL:
 * fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
 *
 */
struct TreeNode *generate_ftree(const char *fname) {
    char empty[] = "";
    struct TreeNode *root = make_node(fname, empty);
    return root;
}

    /*
     * Returns the node at the filepath (path + fname)
     *
     * Iff the filepath exists, make its node.
     * Iff it is a dir, make its children nodes, and keep a pointer in Contents.
     * Iff there are more in the same depth, add a pointer in Next.
     */
struct TreeNode *make_node (const char *fname, char *path) {
    // Create FILEPATH
    char filepath [strlen(path) + strlen(fname) + 1];
    strcpy(filepath, path);
    if(strlen(path) > 0){
        filepath[strlen(path)] = '/';
        filepath[strlen(path) + 1] = '\0';
    }
    strcat(filepath, fname);
    // Create stat struct
    struct stat stat_buf;
    // check if filepath exists
    if (lstat(filepath, &stat_buf) == -1) {
        fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
        return NULL;
    }
    // Create the treenode
    struct TreeNode *node = node_maker(stat_buf, fname, filepath);
    return node;
}

    /*
     * Returns the node created from the given filepath.
     *
     * Creates a TreeNode in the heap, setting all its properties, and, iff
     * it is a dir, creates the nodes of its children. 
     * Returns the pointer to the node.
     */
struct TreeNode *node_maker(struct stat stat_buf, const char *fname, char *filepath) {
    struct TreeNode *node = malloc(sizeof(struct TreeNode));
    // 1. FNAME (FILENAME) (pointer)
    char *filename = malloc(sizeof(char) * (strlen(fname) + 1));
    strcpy(filename, fname);
    node->fname = filename;
    // 2. PERMISSIONS
    node->permissions = stat_buf.st_mode & 0777;  // from lstatdemo.c
    // 3. TYPE
    if (S_ISDIR(stat_buf.st_mode)) {               // IS DIR
        node->type = 'd';
        // 4. CONTENTS
        node->contents = make_children(fname, filepath);
    } else if (S_ISLNK(stat_buf.st_mode)) {     // IS LINK
        node->type = 'l';
    } else if (S_ISREG(stat_buf.st_mode)) {      // IS REG
        node->type = '-';
    }
    return node;
}

    /*
     * Returns the node of the first child.
     *
     * Opens the directory ('filepath') and populates children nodes.
     * If there is an error opening or closing the file, the program will 
     * exit immediately with a stderr message, and return 1.
     */
struct TreeNode *make_children(const char *fname, char *filepath) {
    // open directory from filepath
    DIR *d_ptr = opendir(filepath);
    if (d_ptr == NULL) {
        fprintf(stderr, "The dir (%s) cannot be opened properly!\n", filepath);
        exit(1);
    }
    struct dirent *entry_ptr;
    entry_ptr = readdir(d_ptr);
    // Make Nodes for all children (recursively)
    struct TreeNode *node = children_maker(entry_ptr, d_ptr, fname, filepath);

    if (closedir(d_ptr) == -1){
        fprintf(stderr, "The dir (%s) cannot be closed properly!\n", filepath);
        exit(1);
    };

    return node;
}

    /*
     * Returns the node of the first child. 
     *
     * Creates all children nodes (rescursively), and returns the pointer to the 
     * first for the 'Contents' of the parent directory.
     */
struct TreeNode *children_maker(struct dirent *entry_ptr, DIR *d_ptr, const char *fname, char *path){

    if (entry_ptr == NULL){
        return NULL;
    }
    // Skip prefix '.'
    if (entry_ptr->d_name[0] == '.'){
        entry_ptr = readdir(d_ptr); //next
        return children_maker(entry_ptr, d_ptr, fname, path);
    }
    // make Node
    struct TreeNode *node = make_node(entry_ptr->d_name, path);
    // 5. NEXT (in same level)
    entry_ptr = readdir(d_ptr);
    if (entry_ptr != NULL){
        node->next = children_maker(entry_ptr, d_ptr, fname, path);
    } else {
        node->next = NULL;
    }

    return node;
}

/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 *
 * The only print statements that you may use in this function are:
 * printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions)
 * printf("%s (%c%o)\n", root->fname, root->type, root->permissions)
 *
 */
void print_ftree(struct TreeNode *root) {
    static int depth = 0;

    if (root == NULL){
        return;
    }

    if (root->type == 'd') {
        printf("%*s", depth * 2, "");
        printf("===== %s (%c%d) =====\n", root->fname, root->type, root->permissions);
        if (root->contents != NULL){
            depth++;
            print_ftree(root->contents);
        }
    } else {
        printf("%*s", depth * 2, "");
        printf("%s (%c%d)\n", root->fname, root->type, root->permissions);
    }

    if (root->next != NULL) {
        print_ftree(root->next);
    } else {
        depth--;
    }
}


/* 
 * Deallocate all dynamically-allocated memory in the FTree rooted at node.
 * 
 */
void deallocate_ftree (struct TreeNode *node) {
   
   // Your implementation here.

}
