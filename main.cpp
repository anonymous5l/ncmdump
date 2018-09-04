#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <taglib/mpegfile.h>
#include <taglib/flacfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/id3v2tag.h>
#include <taglib/tag.h>

#include "cJSON.h"

const uint8_t aes_core_key[17]   = { 0x68, 0x7A, 0x48, 0x52, 0x41, 0x6D, 0x73, 0x6F, 0x35, 0x6B, 0x49, 0x6E, 0x62, 0x61, 0x78, 0x57, 0};
const uint8_t aes_modify_key[17] = { 0x23, 0x31, 0x34, 0x6C, 0x6A, 0x6B, 0x5F, 0x21, 0x5C, 0x5D, 0x26, 0x30, 0x55, 0x3C, 0x27, 0x28, 0};

int
base64_decode(uint8_t *in_str, int in_len, uint8_t *out_str) {
	BIO *b64, *bio;
	BUF_MEM *bptr = NULL;
	int counts;
	int size = 0;

	if (in_str == NULL || out_str == NULL)
		return -1;

	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

	bio = BIO_new_mem_buf(in_str, in_len);
	bio = BIO_push(b64, bio);

	size = BIO_read(bio, out_str, in_len);
	out_str[size] = '\0';

	BIO_free_all(bio);
	return size;
}

int
aes128_ecb_decrypt(const uint8_t *key, uint8_t *in, int in_len, uint8_t *out) {
	int outlen;
	int temp;

	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_init(ctx);
	EVP_CIPHER_CTX_set_padding(ctx, 1);

	if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), 0, key, NULL)) {
		return -1;
	}

	if (!EVP_DecryptUpdate(ctx, out, &outlen, in, in_len))
	{
		return -2;
	}

	if (!EVP_DecryptFinal_ex(ctx, out + outlen, &temp)) {
		return -3;
	}

	outlen += temp;

	EVP_CIPHER_CTX_cleanup(ctx);

	return outlen;
}

uint8_t*
build_key_box(uint8_t *key, int key_len) {
	uint8_t *box = (uint8_t*)malloc(256);

	int i;
	for (i = 0; i < 256; ++i)
	{
		box[i] = (uint8_t)i;
	}

	uint8_t swap = 0;
	uint8_t c = 0;
	uint8_t last_byte = 0;
	uint8_t key_offset = 0;

	for (i = 0; i < 256; ++i)
	{
		swap = box[i];
		c = ((swap + last_byte + key[key_offset++]) & 0xff);
		if (key_offset >= key_len) key_offset = 0;
		box[i] = box[c]; box[c] = swap;
		last_byte = c;
	}

	return box;
}

int
process_file(const char *path) {
	uint32_t ulen = 0;
	int i;

	FILE *f = fopen(path, "r");
	fread(&ulen, sizeof(ulen), 1, f);
	if (ulen != (uint32_t)0x4e455443) {
		printf("This is not a Netease Cloud Music copyright protected file. Skipped. \n");
		return 1;
	}
	fread(&ulen, sizeof(ulen), 1, f);
	if (ulen != (uint32_t)0x4d414446) {
		printf("This is not a Netease Cloud Music copyright protected file. Skipped. \n");
		return 1;
	}
	fseek(f, 2, SEEK_CUR);

	uint32_t key_len = 0;
	fread(&key_len, sizeof(key_len), 1, f);

	uint8_t key_data[key_len];
	fread(key_data, key_len, 1, f);
	for (i = 0; i < key_len; i++)
	{
		key_data[i] ^= 0x64;
	}

	int de_key_len = 0;
	uint8_t de_key_data[key_len];
	memset(de_key_data, 0, key_len);

	de_key_len = aes128_ecb_decrypt(aes_core_key, key_data, key_len, de_key_data);

	fread(&ulen, sizeof(ulen), 1, f);

	unsigned char modifyData[ulen];
	fread(modifyData, ulen, 1, f);

	for (i = 0; i < ulen; i++)
	{
		modifyData[i] ^= 0x63;
	}

	// offset header 22
	size_t data_len;
	uint8_t data[ulen];
	uint8_t dedata[ulen];
	cJSON *json_swap;
	cJSON *music_info;
	int artist_len;


	data_len = base64_decode(modifyData + 22, ulen - 22, data);
	data_len = aes128_ecb_decrypt(aes_modify_key, data, data_len, dedata);
	memcpy(dedata, dedata + 6, data_len - 6);
	dedata[data_len - 6] = 0;

	music_info = cJSON_Parse((const char *)dedata);

	// printf("Music Info:\n%s\n", cJSON_Print(music_info));

	json_swap = cJSON_GetObjectItem(music_info, "musicName");
	char *music_name = cJSON_GetStringValue(json_swap);

	json_swap = cJSON_GetObjectItem(music_info, "album");
	char *album = cJSON_GetStringValue(json_swap);

	json_swap = cJSON_GetObjectItem(music_info, "artist");
	artist_len = cJSON_GetArraySize(json_swap);

	char  artist[1024];
	char *artist_temp;
	int   artist_concat = 0;
	memset(artist, 0, 1024);

	for (i = 0; i < artist_len; i++)
	{
		artist_temp = cJSON_GetStringValue(cJSON_GetArrayItem(cJSON_GetArrayItem(json_swap, i), 0));
		artist_concat += sprintf(artist + artist_concat, "%s; ", artist_temp);
	}

	artist[artist_concat - 2] = 0;

	int   bitrate  = cJSON_GetObjectItem(music_info, "bitrate")->valueint;
	int   duration = cJSON_GetObjectItem(music_info, "duration")->valueint;
	char *format   = cJSON_GetStringValue(cJSON_GetObjectItem(music_info, "format"));

	char music_filename[1024];
	memset(music_filename, 0, 1024);
	sprintf(music_filename, "%s.%s", music_name, format);

	// printf("\n     Album: %s\n", album);
	// printf("    Artist: %s\n", artist);
	// printf("    Format: %s\n", format);
	// printf("   Bitrate: %d\n", bitrate);
	// printf("  Duration: %dms\n", duration);
	// printf("Music Name: %s\n\n", music_name);

	printf("%s\n", music_filename);

	// read crc32 check
	fread(&ulen, sizeof(ulen), 1, f);

	fseek(f, 5, SEEK_CUR);

	uint32_t img_len;

	fread(&img_len, sizeof(img_len), 1, f);

	char img_data[img_len];
	fread(img_data, img_len, 1, f);
	// FILE *img = fopen(album_cache_filename, "w");
	// fwrite(img_data, ulen, 1, img);
	// fclose(img);

	uint8_t *box = build_key_box(de_key_data + 17, de_key_len - 17);

	size_t n;
	
	n = 0x8000;
	
	uint8_t buffer[n];

	FILE *fmusic = fopen(music_filename, "w+");

	while (n > 1) {
		n = fread(buffer, 1, n, f);

		for (i = 0; i < n; i++)
		{
			int j = (i + 1) & 0xff;
			buffer[i] ^= box[(box[j] + box[(box[j] + j) & 0xff]) & 0xff];
		}

		fwrite(buffer, n, 1, fmusic);
	}

	fclose(fmusic);
	free(box);
	fclose(f);

	TagLib::File *audioFile;
	TagLib::Tag *tag;

	TagLib::ByteVector vector(img_data, static_cast<unsigned int>(img_len));

	if (strcmp("mp3", format) == 0) {
		audioFile = new TagLib::MPEG::File(music_filename);

		tag = dynamic_cast<TagLib::MPEG::File*>(audioFile)->ID3v2Tag(true);

		TagLib::ID3v2::AttachedPictureFrame *frame = new TagLib::ID3v2::AttachedPictureFrame;

		frame->setMimeType("image/jpeg");
		frame->setPicture(vector);

		dynamic_cast<TagLib::ID3v2::Tag*>(tag)->addFrame(frame);
	} else if (strcmp("flac", format) == 0) {
		audioFile = new TagLib::FLAC::File(music_filename);

		tag = audioFile->tag();

		TagLib::FLAC::Picture *cover = new TagLib::FLAC::Picture;
		cover->setMimeType("image/jpeg");
		cover->setType(TagLib::FLAC::Picture::FrontCover);
		cover->setData(vector);

		dynamic_cast<TagLib::FLAC::File*>(audioFile)->addPicture(cover);
	} else {
		printf("Unknown format\n");
		return 2;
	}

	tag->setTitle(TagLib::String(music_name, TagLib::String::UTF8));
	tag->setArtist(TagLib::String(artist, TagLib::String::UTF8));
	tag->setAlbum(TagLib::String(album, TagLib::String::UTF8));
	tag->setComment(TagLib::String("Create by netease copyright protected dump tool. author 5L", TagLib::String::UTF8));

	audioFile->save();

	cJSON_Delete(music_info);

	return 0;
}

int
main(int argc, char **argv) {
	if (argc <= 1) {
		printf("No file given. \n");
		printf("Usage: ./ncmdump file_1 [file_2 ... file_n]\n");

		return 1;
	}

	for(int i = 0; i < argc - 1; i++) {
		process_file(argv[i + 1]);
	}

	return 0;
}
