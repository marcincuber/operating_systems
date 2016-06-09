/////////////////////////////////////////////////////////////
//////////// Marcin Cuber                    ////////////////
//////////// 3rd year Computer Science MEng  ////////////////
//////////// Operating systems CW2           ////////////////
//////////// Part 1- removing file           ////////////////
/////////////////////////////////////////////////////////////

/* Legend (major terms):
    file_in- file inode structure
    direction_in- directory inode structure
    dir_entry- directory entry
    dir_name- directory name
    in_root- root's inode
    in_file- file's inode
    in_directory- directory's inode
    in_direction2- variable used in order to modify the size, makes code clearer
    name_f- name of the file (i.e. filename)
    name_f_temp- file name with added end of string char also temporary
    filename- only filename stored
    adjust_s- corrects the size of the directory
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fs_disk.h"
#include "jfs_common.h"
//it's defined in fs_disk.h, but it is more readible and easier to use if we define it here as well
#define BSIZE 512 //limits size of the block to 512 bytes

/////////////////////////////////////Remove function//////////////////////////////////

void remove_f (jfs_t *jfs, char *name_f)
{
    struct inode file_in, direction_in;
    struct dirent* dir_entry;
    
    char block [BSIZE];
    char filename [MAX_FILENAME_LEN];
    char dir_name [MAX_FILENAME_LEN];
    char new_block [BSIZE];
    
    // variables for inode root, file, direction, size, and bytes completed
    int in_root;
    int in_file;
    int in_direction;
    int direction_size;
    int comp_bytes;
    
    // variables used while modifying size
    int in_direction2;
    unsigned int adjust_s;
    
	//get name of the file and inode of root directory
    name_f = name_f - 1;
	do
    {
		name_f += 1;
    } while (name_f[0]=='/');
    
    //now we taking path of directory where file is stored. Take the name_f (name of file) and find inode of file
	//additionally many existing functions are reused
    in_root = find_root_directory (jfs);
	all_but_last_part (name_f, dir_name);
	last_part (name_f, filename);
	in_file = findfile_recursive (jfs, name_f, in_root, DT_FILE);
    
    
	//check file's inode and verify the existance of the file, checking only
	if (in_file == -1)
    {
		printf("Cannot remove '%s' because file or directory provided doesn't exists\n", name_f);
        exit(EXIT_FAILURE);
	}
    
    //verification in order to check whether given directory is a root one
	if (strlen(dir_name))
    {
        in_direction = findfile_recursive (jfs, dir_name, in_root, DT_DIRECTORY);
    }
    else
    {
        in_direction = in_root;
	}
    
    
	//following lines gather file's inode, then directory's inode and start reading directory from first block
	get_inode(jfs, in_file, &file_in);
	get_inode(jfs, in_direction, &direction_in);
	direction_size = direction_in.size;
	jfs_read_block(jfs, block, direction_in.blockptrs[0]);
	dir_entry = (struct dirent*) block;
    
    //initialise unused so far variable
    comp_bytes = 0;
    for (;;)
    {
		//new temp char is created which which adds char at the end of string
        //so, we extend length, make copy to memeory and sub-in the \0
		char name_f_temp [MAX_FILENAME_LEN + 1];
		memcpy (name_f_temp, dir_entry->name, dir_entry->namelen);
		name_f_temp [dir_entry->namelen] = '\0';
        
		//succesfully found file follows that we set the inode to free status
        // do while loops through blocks of specific file
		if (!strcmp (name_f_temp, filename))
        {
			return_inode_to_freelist (jfs, in_file);
			int i = 0;
            while (file_in.blockptrs[i])
            {
				//set block to have free status
				return_block_to_freelist(jfs, file_in.blockptrs[i]);
				i = i + 1;
            }
            
            
            // new char need to be declared being used to determine what is before(start) and after(end) specified file(needing to be removed)
            // then we make a copy to memory of new block, old one and completed bytes (initialised to 0)
            // following, we update position of the pointer to the start of data and another pointer to be positioned at the data after file
            // next we copy data to a fresh data-block
            // ending with code-line that writes the new data-block to a "disk" or differently named environment created by ./jfs_format command in terminal
            char *start, *end;
			memcpy (new_block, block, comp_bytes);
			start = (char *)(new_block + comp_bytes);
			int bytes_entrylength = comp_bytes + dir_entry->entry_len;
			end = (char *)(block + bytes_entrylength);
            int temp = BLOCKSIZE - bytes_entrylength;
 			memcpy(start, end, (temp));
			jfs_write_block(jfs, new_block, direction_in.blockptrs[0]);
            
            //now when we dealt with deleting the file, we adopt a correct size to a directory
            //So,firsty we're are reading block where directory is present
            //Then we finding directory inode
            //Following this with correct(new) size that needs to be adjusted
            //Then we write block into "disk" or differently named filesystem
            //finalising with commit function to apply all changes that were made in this function
            in_direction2 = in_direction;
            adjust_s = direction_size - dir_entry->entry_len;
            struct inode *in_direction;
            jfs_read_block(jfs, block, inode_to_block(in_direction2));
            char temp2 = in_direction2 % INODES_PER_BLOCK;
            in_direction = (struct inode *)(block + (temp2)* INODE_SIZE);
            in_direction->size = adjust_s;
            jfs_write_block(jfs, block, inode_to_block(in_direction2));
            jfs_commit(jfs);
			break;
		}
        else
        {
			comp_bytes += dir_entry->entry_len;
			dir_entry = (struct dirent*)(block + comp_bytes);
			//read entire data available in block
			if (comp_bytes >= direction_size)
            {
				break;
			}
		}
	}
}

/////////////////////////////////main function that controls removing of the file /////////////////////////

/* The main function uses the above remove_f function in order to delete file in specified directory. However it also checks whether correct format is correct, if not it returns an error message and exits. Overall, function does its job and deletes files without any problems.
 In terms of functionality it deletes the top listed files in a directory. Meaning that if we have multiple files with the same name in the same directory and if we run ./jfl_ls disk and we get list of files that have the same name; running for example ./jfl_rm disk foo/README will delete the top listed file with the name that maches README in directory foo on environment disk
*/
int main(int length, char **argv)
{
    struct disk_image *directory_main;
    jfs_t *jfs;
    
    // check if called jfs_rm has correct format i.e. argc = 3, if its inccorrectly called or there is a missing part we see worning
    if (length > 3 || length < 3)
    {
        printf("Incorrent format, use the following: jfs_rm <volume_name> <file_name>\n");
        exit(EXIT_FAILURE);
    }

    directory_main = mount_disk_image(argv[1]);
    jfs = init_jfs(directory_main);
	
    //remove file and modify the size of specific directory
	remove_f(jfs,argv[2]);
	unmount_disk_image(directory_main);
    
    exit (EXIT_SUCCESS);
}

