/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/
#define FREE_BLOCK_BITMAP_BLOCK_NO 0
#define INODE_STORING_BLOCK_NO 1

#define FREE_BLOCK_IDENTIFIER 'f'
#define USED_BLOCK_IDENTIFIER 'u'

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/
Inode::Inode()
{
  free_inode_flag = true;
  id = -1;
}

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
    
    no_of_freeblocks = SimpleDisk::BLOCK_SIZE/sizeof(unsigned char);
    free_blocks = new unsigned char[no_of_freeblocks];

    inodes = new Inode[MAX_INODES];
    Console::puts("file system constructed succesfully.\n");
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");

    disk->write(0,free_blocks); //Store free blocks
    unsigned char* temp = (unsigned char*) inodes;
    disk->write(1,temp); // Store inode block 
    
    Console::puts("unmounted file system successfully\n");
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/


bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system from disk\n");
    
    disk = _disk;
    unsigned char * temp;
    unsigned int i;

    // free blocks
    _disk->read(FREE_BLOCK_BITMAP_BLOCK_NO, free_blocks);
    // store inode number in temp
    _disk->read(INODE_STORING_BLOCK_NO, temp);

    inodes = (Inode *) temp;
    inode_ctr= 0;

    for (i ;i < MAX_INODES ;i++){
      if (!inodes[i].free_inode_flag){
        inode_ctr++;
      }
    }

    no_of_freeblocks = SimpleDisk::BLOCK_SIZE/sizeof(unsigned char) ;
    return true;

}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */
    int count_of_free_blocks = _size/SimpleDisk::BLOCK_SIZE;
    unsigned char* array_for_free_blocks = new unsigned char[count_of_free_blocks];
    
    array_for_free_blocks[0] = USED_BLOCK_IDENTIFIER;
    array_for_free_blocks[1] = USED_BLOCK_IDENTIFIER;
    
    for (int i=2;i<count_of_free_blocks;i++){
      array_for_free_blocks[i]=FREE_BLOCK_IDENTIFIER;
    }

    _disk->write(FREE_BLOCK_BITMAP_BLOCK_NO,array_for_free_blocks);
    Inode* temp_inodes = new Inode[MAX_INODES];
    unsigned char* temp = (unsigned char*) temp_inodes;
    _disk->write(INODE_STORING_BLOCK_NO,temp);
    return true;

}

Inode * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you go through the inode list to find the file. */
    for (int i = 0;i++;i<inode_ctr){
      if (inodes[i].id==_file_id){
        return &inodes[i];
      }

    }
    assert(false);
    return NULL;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    
    for (int i = 0;i++;i<inode_ctr){
      if (inodes[i].id==_file_id){
        assert(false);
      }

    }
    
    int free_idx_inode = -1;
    for (int i = 0;i < MAX_INODES;i++){
      if (inodes[i].free_inode_flag){
        free_idx_inode = i;
        break;
      }
    }
    
    int free_idx_block = -1;

    for (int i = 0;i<no_of_freeblocks;i++){
      if (free_blocks[i]==FREE_BLOCK_IDENTIFIER){
        free_idx_block = i;
        break;
      }
    }
    
    // check if inode idx and block idx are not = -1 for free blocks
    bool checkFlag = (free_idx_inode!=-1) && (free_idx_block!=-1);
    if(!checkFlag){
        return false;
    }else{
        inodes[free_idx_inode].fs = this;
        inodes[free_idx_inode].free_inode_flag = false;

        inodes[free_idx_inode].block_no = free_idx_block;
        inodes[free_idx_inode].id = _file_id;

        free_blocks[free_idx_block] = USED_BLOCK_IDENTIFIER;
        
        Console::puts("File with id:"); Console::puti(_file_id); Console::puts("created successfully!\n");
        return true;
    }
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:"); Console::puti(_file_id); Console::puts("\n");

    bool file_found_flag = false;
    int delete_inode_idx;
    unsigned int i = 0;


    for (i = 0;i < MAX_INODES;i++){
      if (inodes[i].id == _file_id){
        // Found the inode index the file is at
        delete_inode_idx = i;
        file_found_flag = true;
        break;
      }
    }

    if(file_found_flag){
        int block_no = inodes[delete_inode_idx].block_no;
        free_blocks[block_no] = FREE_BLOCK_IDENTIFIER;

        inodes[delete_inode_idx].free_inode_flag = true;
        inodes[delete_inode_idx].file_size = 0;

        return true;
    }else{
        return false;
    }
    


}
