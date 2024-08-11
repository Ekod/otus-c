#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

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
    // unknown_length *filename;
    // Дополнительные данные (размером extraFieldLength)
    // unknown_length *extraField;
};
#pragma pack(pop)

struct ZipHelperData
{
    bool is_zip;
    int zip_start_position;
};

long get_file_size(int fileDescriptor)
{
    struct stat statStruct;
    fstat(fileDescriptor, &statStruct);

    return statStruct.st_size;
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

    int fd = fileno(file);

    long fileSize = get_file_size(fd);
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

#pragma pack(push, 1)
struct DataDescriptor
{
    // Контрольная сумма
    uint32_t crc32;
    // Сжатый размер
    uint32_t compressedSize;
    // Несжатый размер
    uint32_t uncompressedSize;
};
#pragma pack(pop)

enum CompresionMethods
{
    no_compression = 0,
    shrunk = 1,
    reduced_with_compression_factor_1 = 2,
    reduced_with_compression_factor_2 = 3,
    reduced_with_compression_factor_3 = 4,
    reduced_with_compression_factor_4 = 5,
    imploded = 6,
    deflated = 8,
    enhanced_deflated = 9,
    PKWare_DCL_imploded = 10,
    compressed_using_BZIP2 = 12,
    LZMA = 14,
    compressed_using_IBM_TERSE = 18,
    IBM_LZ77_z = 19,
    PPMd_version_I_Rev_1 = 98
};

enum Flags
{
    encrypted_file = 0,
    compression_option_1 = 1,
    compression_option_2 = 2,
    data_descriptor = 3,
    enhanced_deflation = 4,
    compressed_patched_data = 5,
    strong_encryption = 6,
    language_encoding = 11,
    mask_header_values = 13,
};

void list_files_names(FILE *file, const int position)
{

    int fd = fileno(file);
    long fileSize = get_file_size(fd);

    fseek(file, position, SEEK_SET);

    uint32_t zipFileHeaderLittleEndianSignature = 0x04034b50;

    while (ftell(file) < fileSize)
    {
        struct ZipLocalFileHeader header;
        fread(&header, sizeof(header), 1, file);

        int nextFileOffset = header.extraFieldLength; // так как поле extraField не используем, то можем сразу закинуть его длину для вычисления сдвига

        if (header.compressionMethod == no_compression)
        {
            nextFileOffset += header.uncompressedSize;
        }
        else
        {
            nextFileOffset += header.compressedSize;
        }

        bool hasDataDescriptor = (header.generalPurposeBitFlag << data_descriptor) & 1;

        if (hasDataDescriptor)
        {
            nextFileOffset += sizeof(struct DataDescriptor);
        }

        if (header.signature == zipFileHeaderLittleEndianSignature)
        {
            char fileName[header.filenameLength + 1];
            fread(fileName, header.filenameLength, 1, file);
            fileName[header.filenameLength] = '\0';

            printf("File: %s\n", fileName);

            fseek(file, nextFileOffset, SEEK_CUR);
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
    if (!file)
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
        printf("file does not have zip format\n");
    }

    fclose(file);

    return 0;
}
