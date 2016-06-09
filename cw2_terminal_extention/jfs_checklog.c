/////////////////////////////////////////////////////////////
//////////// Marcin Cuber                    ////////////////
//////////// 3rd year Computer Science MEng  ////////////////
//////////// Operating systems CW2           ////////////////
//////////// Part 2- fixing directory        ////////////////
/////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fs_disk.h"
#include "jfs_common.h"

#define BSIZE 512

/* The function works correctly, however it only works in case there is only one existing blo_commit. 
   The function is using similir function to rm.c and it does the following. First it finds the log file and then gets its inode. We also check whether the directory exists. Secondly it loads all the available blocks of the given file and stops when it finds the blo_commit. After the commit block is found it then checks if it is flagged with zero and saves blocks in the filesystem (e.g. disk). Lastly it clears all the blocks that were cached in the log file.
 
 */

/////////////////////////////////main function that controls removing of the file /////////////////////////
int main(int length, char **argv)
{
    struct disk_image *directory_main;
    jfs_t *jfs;
    
    // checking whether the correct format was used
    if (length > 2 || length < 2)
    {
        printf("Use the following: jfs_rm <volume_name>\n");
        exit(EXIT_FAILURE);
    }


    directory_main = mount_disk_image(argv[1]);
    jfs = init_jfs(directory_main);
    

    int in_root;
    int in_file;
    int size_file;
    int blocks_num = 0;
    int temp_var;

    struct inode file_in;

    struct cached_block *blo_start = NULL;
    struct cached_block *blo_end = NULL;
    struct cached_block *temp_block = NULL;
    struct commit_block *blo_commited = NULL;
    
    //get inode number of the root directory
    in_root = find_root_directory(jfs);
    //find the inode number of the log file
    in_file = findfile_recursive(jfs, ".log", in_root, DT_FILE);
    
    //check file's inode and verify the existance of the file, checking only
    if (in_file == -1)
    {
        printf("Log file doesn't exits\n");
        exit(EXIT_FAILURE);
    }

    get_inode(jfs, in_file, &file_in);

    size_file = file_in.size;
    
    //initialised unused so far variables
    int comp_bytes = 0;
    int commit_blo_count = 0;

    while (comp_bytes < size_file)
    {
        temp_block = malloc(sizeof(struct cached_block));
        temp_block->next = NULL;

        if (blo_end == NULL)
        {
            blo_start = blo_end = temp_block;
        }
        else {
            blo_end->next = temp_block;
            blo_end = temp_block;
        }

        jfs_read_block(jfs, temp_block->blockdata, file_in.blockptrs[blocks_num]);

        blo_commited = (struct commit_block*)temp_block->blockdata;
        
        
        //check if the block is commited by checking it with the magic number
        if (blo_commited->magicnum == 0x89abcdef)
        {
            unsigned int temp_sum = 0;
            for (temp_var = 0; temp_var < blocks_num; ++temp_var)
            {
                temp_sum += blo_commited->blocknums[temp_var];
            }

            if(blo_commited->sum == temp_sum)
            {
                commit_blo_count = 1;
                break;
            }
        }
        blocks_num = blocks_num + 1;
        
        comp_bytes = comp_bytes + BSIZE;
    }

    if (commit_blo_count)
    {
        if (blo_commited->uncommitted == 0)
        {
            exit(EXIT_SUCCESS); // end with success as log is fully commited
        }

        //write all of the blocks that were not saved
        temp_block = blo_start;
        for (temp_var = 0; temp_var < blocks_num; ++temp_var)
        {
            jfs_write_block(jfs, temp_block->blockdata, blo_commited->blocknums[temp_var]);
            temp_block = temp_block->next;
        }
        //apply all the changes
        jfs_commit(jfs);
    }

    
    // temporary block is assigned with the starting block
    temp_block = blo_start;
    for (temp_var = 0; temp_var < blocks_num; ++temp_var)
    {
        blo_start = temp_block;
        temp_block = temp_block->next;
        free(blo_start);
    }

    unmount_disk_image(directory_main);

    exit(EXIT_SUCCESS);
}

