#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int largura;
    int altura;
    int max_cor;
    unsigned char *dados;
} Imagem;

Imagem* carregar_ppm(const char *nome_arquivo) {
    FILE *fp = fopen(nome_arquivo, "rb");
    if (!fp) return NULL;

    char formato[3];
    if (fscanf(fp, "%2s", formato) != 1 || formato[0] != 'P' || formato[1] != '6') {
        fclose(fp);
        return NULL;
    }

    int c = getc(fp);
    while (c == '#' || c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        if (c == '#') {
            while (getc(fp) != '\n');
        }
        c = getc(fp);
    }
    ungetc(c, fp);

    Imagem *img = (Imagem*)malloc(sizeof(Imagem));
    if (fscanf(fp, "%d %d %d", &img->largura, &img->altura, &img->max_cor) != 3) {
        free(img);
        fclose(fp);
        return NULL;
    }
    fgetc(fp);

    int total_bytes = img->largura * img->altura * 3;
    img->dados = (unsigned char*)malloc(total_bytes * sizeof(unsigned char));
    fread(img->dados, sizeof(unsigned char), total_bytes, fp);

    fclose(fp);
    return img;
}

int salvar_ppm(const char *nome_arquivo, Imagem *img) {
    FILE *fp = fopen(nome_arquivo, "wb");
    if (!fp) return 0;

    fprintf(fp, "P6\n%d %d\n%d\n", img->largura, img->altura, img->max_cor);
    int total_bytes = img->largura * img->altura * 3;
    fwrite(img->dados, sizeof(unsigned char), total_bytes, fp);

    fclose(fp);
    return 1;
}

void liberar_imagem(Imagem *img) {
    if (img) {
        if (img->dados) free(img->dados);
        free(img);
    }
}

void gerar_imagem_teste(const char *nome_arquivo, int largura, int altura) {
    Imagem img;
    img.largura = largura;
    img.altura = altura;
    img.max_cor = 255;
    int total_bytes = largura * altura * 3;
    img.dados = (unsigned char*)malloc(total_bytes * sizeof(unsigned char));

    unsigned char *ptr = img.dados;
    for (int y = 0; y < altura; y++) {
        for (int x = 0; x < largura; x++) {
            *ptr = (unsigned char)((x * 255) / largura);
            *(ptr + 1) = (unsigned char)((y * 255) / altura);
            *(ptr + 2) = (unsigned char)(((x + y) * 255) / (largura + altura));
            ptr += 3;
        }
    }

    salvar_ppm(nome_arquivo, &img);
    free(img.dados);
}

void trocar_pixels(unsigned char *p1, unsigned char *p2) {
    unsigned char temp;
    for (int c = 0; c < 3; c++) {
        temp = *(p1 + c);
        *(p1 + c) = *(p2 + c);
        *(p2 + c) = temp;
    }
}

void aplicar_confusao(Imagem *img) {
    int total_pixels = img->largura * img->altura;
    unsigned char *ptr = img->dados;

    for (int i = 0; i < total_pixels; i++) {
        *ptr = 255 - (*ptr);
        *(ptr + 1) = (unsigned char)((*(ptr + 1) + 128) % 256);
        *(ptr + 2) = *(ptr + 2) ^ 85;
        ptr += 3;
    }
}

void reverter_confusao(Imagem *img) {
    int total_pixels = img->largura * img->altura;
    unsigned char *ptr = img->dados;

    for (int i = 0; i < total_pixels; i++) {
        *ptr = 255 - (*ptr);
        *(ptr + 1) = (unsigned char)((*(ptr + 1) - 128 + 256) % 256);
        *(ptr + 2) = *(ptr + 2) ^ 85;
        ptr += 3;
    }
}

void aplicar_difusao(Imagem *img) {
    int total_pixels = img->largura * img->altura;
    unsigned char *p_inicio = img->dados;
    unsigned char *p_fim = img->dados + (total_pixels - 1) * 3;

    while (p_inicio < p_fim) {
        trocar_pixels(p_inicio, p_fim);
        p_inicio += 3;
        p_fim -= 3;
    }

    unsigned char *ptr = img->dados;
    for (int i = 0; i + 3 < total_pixels; i += 4) {
        trocar_pixels(ptr, ptr + 6);
        trocar_pixels(ptr + 3, ptr + 9);
        ptr += 12;
    }
}

void reverter_difusao(Imagem *img) {
    int total_pixels = img->largura * img->altura;
    unsigned char *ptr = img->dados;

    for (int i = 0; i + 3 < total_pixels; i += 4) {
        trocar_pixels(ptr, ptr + 6);
        trocar_pixels(ptr + 3, ptr + 9);
        ptr += 12;
    }

    unsigned char *p_inicio = img->dados;
    unsigned char *p_fim = img->dados + (total_pixels - 1) * 3;

    while (p_inicio < p_fim) {
        trocar_pixels(p_inicio, p_fim);
        p_inicio += 3;
        p_fim -= 3;
    }
}

int verificar_igualdade(Imagem *img1, Imagem *img2) {
    if (img1->largura != img2->largura || img1->altura != img2->altura) {
        return 0;
    }

    int total_bytes = img1->largura * img1->altura * 3;
    unsigned char *p1 = img1->dados;
    unsigned char *p2 = img2->dados;

    for (int i = 0; i < total_bytes; i++) {
        if (*p1 != *p2) {
            return 0;
        }
        p1++;
        p2++;
    }
    return 1;
}

int main() {
    Imagem *img_original = carregar_ppm("original.ppm");
    Imagem *img_processada = carregar_ppm("original.ppm");

    if (!img_original || !img_processada) {
        printf("Erro ao carregar as imagens.\n");
        return 1;
    }

    aplicar_confusao(img_processada);
    aplicar_difusao(img_processada);
    salvar_ppm("baguncada.ppm", img_processada);

    reverter_difusao(img_processada);
    reverter_confusao(img_processada);
    salvar_ppm("recuperada.ppm", img_processada);

    if (verificar_igualdade(img_original, img_processada)) {
        printf("SUCESSO: 100%% dos bytes recuperados com exatidao!\n");
    } else {
        printf("ERRO: A imagem recuperada difere da original.\n");
    }

    liberar_imagem(img_original);
    liberar_imagem(img_processada);

    return 0;
}