#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
	uint8_t *mapped_data;
	size_t mapped_size;
} MRead;

typedef struct {
	uint8_t *mapped_data;
	size_t mapped_size;
	int fd;
} MWrite;

static inline 
MRead
mmap_file_open_ro(
	const char* file_name
) {
	uint8_t *mapped_data;
	size_t mapped_size;

	int fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		printf("Can't open file %s\n",file_name);
		exit(1);
	}

	struct stat statbuf;
	int err = fstat(fd, &statbuf);
	if (err < 0) {
		printf("Can't fstat file %s\n",file_name);
		exit(1);
	}

	mapped_size = statbuf.st_size;
	mapped_data = (uint8_t*)mmap( NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0 );
	if (mapped_data == MAP_FAILED) {
		printf("Can't mmap file %s\n",file_name);
		exit(1);
	}
	close( fd );

	MRead m_read = {
		.mapped_data = mapped_data,
		.mapped_size = mapped_size
	};

	return m_read;
}

static inline 
void
mmap_file_close(
	MRead m_read
) {
	int err = munmap(m_read.mapped_data, m_read.mapped_size);
	if (err != 0) {
		printf("Could not munmap at %p\n", m_read.mapped_data);
		exit(1);
	}
}

static inline
MWrite
mmap_file_open_wo(
	const char* file_name, 
	size_t mapped_size
) {
	uint8_t *mapped_data;
	int fd;

	fd = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
        perror("Failed to open file");
        exit(1);
    }

    // Resize the file to the size of the array
    if (ftruncate(fd, mapped_size) == -1) {
        perror("Failed to resize file");
        close(fd);
        exit(1);
    }

	mapped_data = (uint8_t*)mmap(NULL, mapped_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_data == MAP_FAILED) {
        perror("Failed to map file");
        close(fd);
        exit(1);
    }

	MWrite m_write = {
		.mapped_data = mapped_data,
		.mapped_size = mapped_size,
		.fd = fd
	};

	return m_write;
}

static inline void
mmap_file_sync_close(
	MWrite m_write
) {
	if (msync(m_write.mapped_data, m_write.mapped_size, MS_SYNC) == -1) {
        perror("Failed to sync memory");
    }

	int err = munmap(m_write.mapped_data, m_write.mapped_size);
	if (err != 0) {
		printf("Could not munmap at %p\n", m_write.mapped_data);
		exit(1);
	}

	close(m_write.fd);
}

