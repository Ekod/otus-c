#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#pragma pack(push, 1)
struct ZipLocalFileHeader
{
    // Обязательная сигнатура, равна 0x04034b50
    uint32_t signature;
    // Минимальная версия для распаковки
    uint16_t versionToExtract;
    // Битовый флаг
    uint16_t generalPurposeBitFlag;
    // Метод сжатия (0 - без сжатия, 8 - deflate)
    uint16_t compressionMethod;
    // Время модификации файла
    uint16_t modificationTime;
    // Дата модификации файла
    uint16_t modificationDate;
    // Контрольная сумма
    uint32_t crc32;
    // Сжатый размер
    uint32_t compressedSize;
    // Несжатый размер
    uint32_t uncompressedSize;
    // Длина название файла
    uint16_t filenameLength;
    // Длина поля с дополнительными данными
    uint16_t extraFieldLength;
    // Название файла (размером filenameLength)
};

struct ZipHelperData
{
    bool is_zip;
    int zip_start_position;
};

int get_file_size(FILE *file)
{
    fseek(file, 0, SEEK_END);
    return ftell(file);
}

struct ZipHelperData is_zip(FILE *file)
{
    const char *zip_header = "\x50\x4B\x03\x04";
    char header[4];

    struct ZipHelperData zip;
    fread(header, 4, 1, file);
    if (memcmp(header, zip_header, 4) == 0)
    {
        zip.is_zip = true;
        zip.zip_start_position = 0;

        return zip;
    }

    long fileSize = get_file_size(file);
    for (int i = 1; i < fileSize; i++)
    {
        fseek(file, i, SEEK_SET);
        fread(header, 4, 1, file);

        if (memcmp(header, zip_header, 4) == 0)
        {
            zip.is_zip = true;
            zip.zip_start_position = i;

            return zip;
        }
    }

    zip.is_zip = false;
    zip.zip_start_position = -1;

    return zip;
}

void list_files_names(FILE *file, int position)
{

    long fileSize = get_file_size(file);
    fseek(file, position, SEEK_SET);

    while (ftell(file) < fileSize)
    {
        struct ZipLocalFileHeader header;
        fread(&header, sizeof(header), 1, file);

        if (header.signature == 0x04034b50)
        {
            char fileName[header.filenameLength + 1];
            fread(fileName, header.filenameLength, 1, file);
            fileName[header.filenameLength] = '\0';
            printf("File: %s\n", fileName);
            fseek(file, header.extraFieldLength + header.compressedSize, SEEK_CUR);
        }
        else
        {
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        return 1;
    }

    const char *filename = argv[1];
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("couldn't open file");
        return 1;
    }

    struct ZipHelperData zip = is_zip(file);
    if (zip.is_zip)
    {
        printf("file has zip format\n");
        list_files_names(file, zip.zip_start_position);
    }
    else
    {
        printf("file does not have zip format");
    }

    fclose(file);

    return 0;
}

#pragma pack(pop)
