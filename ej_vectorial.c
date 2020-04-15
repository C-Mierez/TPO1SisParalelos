#include <stdlib.h>
#include <stdio.h>
#include <immintrin.h>
// Cantidad de segundos en un dia: 86400
#define VALORES_POR_SENSOR 86400

// Cantidad de sensores de humedad del suelo
#define CANT_SENSORES_HUM_SUELO 2000
// Seed de generación de numeros pseudo-aleatorios
#define SEED 1u

// Para los datos de los sensores usamos doubles (8 bytes)
#define type_sensor double
// Para los resultados de los sensores usamos char (1 byte)
#define type_result char
// Para el contador de resultados correctos e incorrectos usamos int (4 bytes)
#define type_counter int
// Utilizaremos registros de 256 bits => __m256d

// TODO Restar la cantidad de correctos a la cantidad de segundos por dia para obtener los incorrectos

int main()
{

    // Declaracion de vectores iniciales
    type_sensor *vec_tempAire, *vec_humAire;
    type_sensor **vec_vec_humSuelo;
    // Declaracion de vectores resultado
    type_result *vec_res_tempAire, *vec_res_humAire;
    type_result **vec_vec_res_humSuelo;

    // Declaracion de variables
    int i, x, cantElementosPorRegistro, cantIteracionesPorRegistro;
    int type_sensor_size;
    double rand_num;

    type_sensor_size = sizeof(type_sensor);                                     // 8
    cantElementosPorRegistro = sizeof(__m256d) / type_sensor_size;              // 4
    cantIteracionesPorRegistro = VALORES_POR_SENSOR / cantElementosPorRegistro; // 21600

    // Seed del rand
    srand(SEED);

    //! COMIENZO INICIALIZACION

    // Allocate memoria para los vectores de los valores de sensores de humedad del suelo

    rand_num = 90.0;
    if (posix_memalign((void **)&vec_tempAire, 32, VALORES_POR_SENSOR * type_sensor_size) != 0)
    {
        return 1;
    }

    for (x = 0; x < VALORES_POR_SENSOR; x++)
    { // TODO Loop Unrolling
        // Llenamos los vectores con valores random entre -40 y 50
        vec_tempAire[x] = (type_sensor)rand() / ((type_sensor)RAND_MAX) * rand_num - 40;
    }

    rand_num = 100.0;
    if (posix_memalign((void **)&vec_humAire, 32, VALORES_POR_SENSOR * type_sensor_size) != 0)
    {
        return 1;
    }
    for (x = 0; x < VALORES_POR_SENSOR; x++)
    { // TODO Loop Unrolling
        // Llenamos los vectores con valores random entre 0 y 100
        vec_humAire[x] = (type_sensor)rand() / ((type_sensor)RAND_MAX) * rand_num;
    }

    //vec_vec_humSuelo = malloc(CANT_SENSORES_HUM_SUELO * type_sensor_size);
    if (posix_memalign((void **)&vec_vec_humSuelo, 32, CANT_SENSORES_HUM_SUELO * sizeof(type_sensor *)) != 0)
    {
        return 1;
    }

    for (i = 0; i < CANT_SENSORES_HUM_SUELO; i++)
    { // TODO Loop Unrolling
        // Allocate memoria para los valores de los sensores de humedad del suelo
        //vec_vec_humSuelo[i] = malloc(VALORES_POR_SENSOR * type_sensor_size);
        if (posix_memalign((void **)&vec_vec_humSuelo[i], 32, VALORES_POR_SENSOR * type_sensor_size) != 0)
        {
            return 1;
        }
        for (x = 0; x < VALORES_POR_SENSOR; x++)
        { // TODO Loop Unrolling
            // Llenamos los vectores con valores random entre 0 y 100
            vec_vec_humSuelo[i][x] = (type_sensor)rand() / ((type_sensor)RAND_MAX) * rand_num;
        }
    }

    // Vectores de Resultado
    if (posix_memalign((void **)&vec_res_humAire, 32, VALORES_POR_SENSOR * type_sensor_size) != 0)
    {
        return 1;
    }

    //! FIN INICIALIZACION

    //* Humedad del Aire
    // Creamos el valor del registro a ser usado para comparar
    type_sensor *cond_humAire;
    if (posix_memalign((void **)&cond_humAire, 32, cantElementosPorRegistro * type_sensor_size * 3) != 0)
    {
        return 1;
    }
    for (i = 0; i < cantElementosPorRegistro; i++)
    {
        cond_humAire[i] = 75.0;
    }
    x = cantElementosPorRegistro * 2;
    for (i = cantElementosPorRegistro; i < x; i++)
    {
        cond_humAire[i] = 1;
    }
    x = cantElementosPorRegistro * 3;
    for (i = cantElementosPorRegistro * 2; i < x; i++)
    {
        cond_humAire[i] = 0;
    }

    // Cargar el valor de los 4 elementos en el registro
    __m256d reg_comp_a, reg_comp_b, reg_c, reg_acum;
    reg_comp_a = _mm256_load_pd((type_sensor const *)vec_humAire);
    reg_comp_b = _mm256_load_pd((type_sensor const *)cond_humAire + (cantElementosPorRegistro * type_sensor_size));
    reg_acum = _mm256_load_pd((type_sensor const *)cond_humAire + (cantElementosPorRegistro * type_sensor_size * 2));

    /* Esto es lo que hacemos originalmente pero lo comentamos por ahora para testear
    for (i = 0; i < cantIteracionesPorRegistro; i++)
    {
        reg_c = _mm256_load_pd((type_sensor const *)vec_humAire + (i * cantElementosPorRegistro));
        reg_c = _mm256_cmp_pd(reg_comp_a, reg_c, 2); // 2 => Less or Equal
        reg_c = _mm256_and_pd(reg_c, reg_comp_b);
        reg_acum = _mm256_add_pd(reg_c, reg_acum);
    }*/

    //reg_acum = _mm256_cmp_pd(reg_comp_b,reg_comp_a,18);

    _mm256_store_pd((type_sensor *)vec_res_humAire, reg_comp_a);

    double acum = 0;
    for (i = 0; i < cantElementosPorRegistro; i++)
    {
        //acum += vec_res_humAire[i];
        printf("%lf ", vec_res_humAire[i]);
    }

    //printf("%lf", acum);

    return 0;
}