#include <iostream>
#include <fstream>
#include <cassert>
#include <memory>
#include <cstring>

#include <openssl/bio.h>
#include <openssl/evp.h>

#include <taglib/mpegfile.h>
#include <taglib/flacfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/id3v2tag.h>
#include <taglib/tag.h>

#include "cJSON.h"

const std::uint8_t aes_core_key[17] = {0x68, 0x7A, 0x48, 0x52, 0x41, 0x6D, 0x73, 0x6F, 0x35, 0x6B, 0x49, 0x6E, 0x62,
                                       0x61, 0x78, 0x57, 0};
const std::uint8_t aes_modify_key[17] = {0x23, 0x31, 0x34, 0x6C, 0x6A, 0x6B, 0x5F, 0x21, 0x5C, 0x5D, 0x26, 0x30, 0x55,
                                         0x3C, 0x27, 0x28, 0};

int base64_decode(const std::uint8_t* in_str, int in_len, std::uint8_t* out_str)
{
    assert(in_str);
    assert(out_str);
    if(!in_str || !out_str)
        throw std::runtime_error("in_str/out_str");

    BIO* b64, * bio;
    int size = 0;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    bio = BIO_new_mem_buf(in_str, in_len);
    bio = BIO_push(b64, bio);

    size = BIO_read(bio, out_str, in_len);
    out_str[size] = '\0';

    BIO_free_all(bio);
    return size;
}

int aes128_ecb_decrypt(const std::uint8_t* key, std::uint8_t* in, int in_len, std::uint8_t* out)
{
    int out_len;
    int temp;

    EVP_CIPHER_CTX* x = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_set_padding(x, 1);

    if(!EVP_DecryptInit_ex(x, EVP_aes_128_ecb(), nullptr, key, nullptr))
        throw std::runtime_error("EVP_DecryptInit_ex");

    if(!EVP_DecryptUpdate(x, out, &out_len, in, in_len))
        throw std::runtime_error("EVP_DecryptUpdate");

    if(!EVP_DecryptFinal_ex(x, out + out_len, &temp))
        throw std::runtime_error("EVP_DecryptFinal_ex");

    out_len += temp;

    EVP_CIPHER_CTX_free(x);
  
    return out_len;
}

std::unique_ptr<std::uint8_t[]> build_key_box(const std::uint8_t* key, int key_len)
{
    std::unique_ptr<std::uint8_t[]> box(new std::uint8_t[256]{0});

    for(int i = 0; i < 256; ++i)
        box.get()[i] = static_cast<std::uint8_t>(i);

    std::uint8_t swap = 0;
    std::uint8_t c = 0;
    std::uint8_t last_byte = 0;
    std::uint8_t key_offset = 0;

    for(int i = 0; i < 256; ++i)
    {
        swap = box.get()[i];
        c = ((swap + last_byte + key[key_offset++]) & static_cast<std::uint8_t>(0xff));
        if(key_offset >= key_len)
            key_offset = 0;
        box.get()[i] = box.get()[c];
        box.get()[c] = swap;
        last_byte = c;
    }

    return std::move(box);
}

int process_file(const char* path)
{
    std::uint32_t ulen = 0;

    std::ifstream f(path, std::ios::in | std::ios::binary);
    f.read(reinterpret_cast<char*>(&ulen), sizeof(ulen));
    if(ulen != 0x4e455443)
        throw std::runtime_error("isn't netease cloud music copyright file!\n");

    f.read(reinterpret_cast<char*>(&ulen), sizeof(ulen));
    if(ulen != 0x4d414446)
        throw std::runtime_error("isn't netease cloud music copyright file!\n");

    f.ignore(2);

    std::uint32_t key_len = 0;
    f.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));

    std::unique_ptr<std::uint8_t[]> key_data(new std::uint8_t[key_len]{0});
    f.read(reinterpret_cast<char*>(key_data.get()), key_len);
    for(std::uint32_t i = 0; i < key_len; ++i)
    {
        key_data.get()[i] ^= 0x64;
    }

    int de_key_len = 0;
    std::unique_ptr<std::uint8_t[]> de_key_data(new std::uint8_t[key_len]{0});

    de_key_len = aes128_ecb_decrypt(aes_core_key, key_data.get(), key_len, de_key_data.get());

    f.read(reinterpret_cast<char*>(&ulen), sizeof(ulen));

    std::unique_ptr<std::uint8_t[]> modifyData(new std::uint8_t[ulen]{0});
    f.read(reinterpret_cast<char*>(modifyData.get()), ulen);

    for(std::uint32_t i = 0; i < ulen; ++i)
    {
        modifyData.get()[i] ^= 0x63;
    }

    // offset header 22
    int data_len;
    std::unique_ptr<std::uint8_t[]> data(new std::uint8_t[ulen]{0});
    std::unique_ptr<std::uint8_t[]> dedata(new std::uint8_t[ulen]{0});
    cJSON* json_swap;
    cJSON* music_info;
    int artist_len;

    data_len = base64_decode(modifyData.get() + 22, ulen - 22, data.get());
    data_len = aes128_ecb_decrypt(aes_modify_key, data.get(), data_len, dedata.get());
    std::memmove(dedata.get(), dedata.get() + 6, data_len - 6);
    dedata.get()[data_len - 6] = 0;

    music_info = cJSON_Parse(reinterpret_cast<char*>(dedata.get()));

    // printf("Music Info:\n%s\n", cJSON_Print(music_info));

    json_swap = cJSON_GetObjectItem(music_info, "musicName");
    char* music_name = cJSON_GetStringValue(json_swap);

    json_swap = cJSON_GetObjectItem(music_info, "album");
    char* album = cJSON_GetStringValue(json_swap);

    json_swap = cJSON_GetObjectItem(music_info, "artist");
    artist_len = cJSON_GetArraySize(json_swap);

    char artist[1024]{0};
    char* artist_temp;
    int artist_concat = 0;

    for(int i = 0; i < artist_len; i++)
    {
        artist_temp = cJSON_GetStringValue(cJSON_GetArrayItem(cJSON_GetArrayItem(json_swap, i), 0));
        artist_concat += std::sprintf(artist + artist_concat, "%s&", artist_temp);
    }

    artist[artist_concat - 2] = 0;

    // int bitrate = cJSON_GetObjectItem(music_info, "bitrate")->valueint;
    // int duration = cJSON_GetObjectItem(music_info, "duration")->valueint;
    char* format = cJSON_GetStringValue(cJSON_GetObjectItem(music_info, "format"));

    char music_filename[1024]{0};
    std::sprintf(music_filename, "%s - %s.%s", artist, music_name, format);

    // printf("\n     Album: %s\n", album);
    // printf("    Artist: %s\n", artist);
    // printf("    Format: %s\n", format);
    // printf("   Bitrate: %d\n", bitrate);
    // printf("  Duration: %dms\n", duration);
    // printf("Music Name: %s\n\n", music_name);

    std::cout << music_filename << std::endl;

    // read crc32 check
    f.read(reinterpret_cast<char*>(&ulen), sizeof(ulen));
    f.ignore(5);

    std::uint32_t img_len{0};
    f.read(reinterpret_cast<char*>(&img_len), sizeof(img_len));

    std::unique_ptr<char[]> img_data(new char[img_len]);
    f.read(img_data.get(), img_len);
    // FILE *img = fopen(album_cache_filename, "w");
    // fwrite(img_data, ulen, 1, img);
    // fclose(img);

    auto box = build_key_box(de_key_data.get() + 17, de_key_len - 17);

    std::streamsize n = 0x8000;
    std::unique_ptr<std::uint8_t[]> buffer(new std::uint8_t[n]{0});

    std::ofstream fmusic(music_filename, std::ios::out | std::ios::binary);

    while(n > 1)
    {
        f.read(reinterpret_cast<char*>(buffer.get()), n);
        n = f.gcount();
        for(int i = 0; i < n; i++)
        {
            int j = (i + 1) & 0xff;
            buffer.get()[i] ^= box.get()[(box.get()[j] + box.get()[(box.get()[j] + j) & 0xff]) & 0xff];
        }

        fmusic.write(reinterpret_cast<char*>(buffer.get()), n);
    }

    fmusic.flush();
    fmusic.close();
    f.close();

    std::unique_ptr<TagLib::File> audioFile;
    TagLib::Tag* tag; // raw pointer

    TagLib::ByteVector vector(img_data.get(), static_cast<unsigned int>(img_len));
    auto cover = new TagLib::FLAC::Picture;
    std::unique_ptr<TagLib::ID3v2::AttachedPictureFrame> frame(new TagLib::ID3v2::AttachedPictureFrame);

    if(std::char_traits<char>::compare(format, "mp3", 3) == 0)
    {
        // C++14 std::make_unique
        audioFile.reset(new TagLib::MPEG::File(music_filename)); // NOLINT(modernize-make-unique)
        tag = dynamic_cast<TagLib::MPEG::File*>(audioFile.get())->ID3v2Tag(true); // NOLINT(modernize-make-unique)

        frame->setMimeType("image/jpeg");
        frame->setPicture(vector);

        dynamic_cast<TagLib::ID3v2::Tag*>(tag)->addFrame(frame.get());
    }
    else if(std::char_traits<char>::compare(format, "flac", 4) == 0)
    {
        audioFile.reset(new TagLib::FLAC::File(music_filename)); // NOLINT(modernize-make-unique)
        tag = audioFile->tag(); // NOLINT(modernize-make-unique)

        cover->setMimeType("image/jpeg");
        cover->setType(TagLib::FLAC::Picture::FrontCover);
        cover->setData(vector);

        dynamic_cast<TagLib::FLAC::File*>(audioFile.get())->addPicture(cover);
    }
    else
        throw std::runtime_error("unknown format of media");

    tag->setTitle(TagLib::String(music_name, TagLib::String::UTF8));
    tag->setArtist(TagLib::String(artist, TagLib::String::UTF8));
    tag->setAlbum(TagLib::String(album, TagLib::String::UTF8));
    tag->setComment(TagLib::String("", TagLib::String::UTF8));

    audioFile->save();

    cJSON_Delete(music_info);

    return 0;
}

int main(int argc, char* argv[])
{
    if (argc <= 1) {
        printf("please input file path!\n");
        return 1;
    }

    for(int i = 0; i < argc - 1; i++) {
        process_file(argv[i + 1]);
    }

    return 0;
}