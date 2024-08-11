#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

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
    uint8_t *filename;
    // Дополнительные данные (размером extraFieldLength)
    uint8_t *extraField;
};

struct ZipHelperData /*TODO come up with a better name*/
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

void list_files(FILE *file, int position)
{
    fseek(file, position, SEEK_SET);

    struct ZipLocalFileHeader header;
    fread(&header, sizeof(header), 1, file);

    char fileName[header.filenameLength];
    fread(fileName, header.filenameLength, 1, file);
    printf("File: %s\n", fileName);
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
        list_files(file, zip.zip_start_position);
    }

    fclose(file);

    return 0;
}
