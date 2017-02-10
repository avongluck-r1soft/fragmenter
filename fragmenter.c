/*
 * Fragmenter - Artificial filesystem fragmentation tool
 * Copyright, 2016-2017 Continuum, LLC.
 *
 * Released under the terms of the MIT license
 *
 * Authors:
 *   Alexander von Gluck IV <Alex.vongluck@r1soft.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

#include <linux/limits.h>


static void
get_urand(char* target, int bytes)
{
	int randomDataHandle = open("/dev/urandom", O_RDONLY);
	size_t randomDataPos = 0;
	while (randomDataPos < bytes)
	{
		ssize_t result = read(randomDataHandle, target + randomDataPos,
			bytes - randomDataPos);
		if (result < 0) {
			close(randomDataHandle);
			return;
		}

		randomDataPos += result;
	}
	close(randomDataHandle);
}


static void
gen_random_ascii(char *s, const int len)
{
	static const char alphanum[] =
		"0123456789"
		"abcdefghijklmnopqrstuvwxyz";
	for (int i = 0; i < len; ++i) {
		char randomChar;
		get_urand(&randomChar, 1);
		s[i] = alphanum[randomChar % (sizeof(alphanum) - 1)];
	}

	s[len] = 0;
}


static void
create_file(char* location, size_t size)
{
	printf("Create %s = %ld\n", location, size);
	size_t pos = 0;
	int fh = open(location, O_WRONLY | O_CREAT, S_IRWXU);
	while (pos < size) {
		char randChars[1024];
		get_urand(randChars, 1024);
		pos += write(fh, &randChars, sizeof(randChars));
	}

	// Prevent delayed allocation
	fsync(fh);

	close(fh);
}


// Extend a file with random data
static void
grow_file(char* location, size_t size)
{
	printf("Grow   %s + %ld\n", location, size);
	size_t pos=0;
	int fh = open(location, O_WRONLY | O_APPEND, S_IRWXU);
	while (pos < size) {
		char randChars[1024];
		get_urand(randChars, 1024);
		pos += write(fh, &randChars, sizeof(randChars));
	}

	// Prevent delayed allocation
	fsync(fh);

	close(fh);
}


// Create random files full of random data
static void
create_randoms(char* location, int count, int max_size)
{
	char randomName[PATH_MAX];
	int i;
	for (i = 0; i < count; i++) {
		gen_random_ascii(randomName, 25);
		char fileName[PATH_MAX];
		snprintf(fileName, PATH_MAX - 1, "%s/%s", location, randomName);

		int size = rand() % max_size;
		create_file(fileName, size);
	}
}


// Create random files full of random data
static void
grow_randoms(char* location, int count, int max_size)
{
	DIR *dir;
	struct dirent* ent;

	if ((dir = opendir(location)) != NULL) {
		int grown = 0;
		while (((ent = readdir (dir)) != NULL) && grown < count) {
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
				continue;
			char fileName[PATH_MAX];
			snprintf(fileName, PATH_MAX - 1, "%s/%s", location, ent->d_name);
			int size = rand() % max_size;
			grow_file(fileName, size);
			grown++;
		}
	}
}


// Find files in the path given and delete x random files
// (this one is a bit scary :-O)
static void
unlink_randoms(char* location, int count)
{
	DIR *dir;
	struct dirent* ent;

	if ((dir = opendir(location)) != NULL) {
		int unlinked = 0;
		while (((ent = readdir (dir)) != NULL) && unlinked < count) {
			if (strcmp(ent->d_name, ".") == 0
				|| strcmp(ent->d_name, "..") == 0
				|| strcmp(ent->d_name, "fragmented") == 0)
				continue;

			char fileName[PATH_MAX];
			snprintf(fileName, PATH_MAX - 1, "%s/%s", location, ent->d_name);
			printf ("Unlink %s\n", fileName);
			unlink(fileName);
			unlinked++;
		}
	}
}


int
main(int argc, char* argv[])
{
	srand(time(NULL));

	if (argc != 2) {
		printf("Usage: %s <path>\n", argv[0]);
		return 1;
	}

	struct stat pathStat;
	int err = stat(argv[1], &pathStat);

	if (err < 0 || !S_ISDIR(pathStat.st_mode)) {
		printf("Error: Invalid path provided '%s'\n", argv[1]);
		return 1;
	}

	// Create fragmented file
	char fragmented[PATH_MAX];
	snprintf(fragmented, PATH_MAX, "%s/fragmented", argv[1]);

	create_file(fragmented, 1050000000);

	int loops = 0;
	while (loops < 100) {
		// Create "small ones"
		create_randoms(argv[1], 2, 5240000);
		// Grow fragmented one
		grow_file(fragmented, rand() % 105000000);
		// Grow random files
		grow_randoms(argv[1], rand() % 5, 10500000);

		loops++;
	}

	return 0;
}
