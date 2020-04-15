#include <stdlib.h>
#include <stdio.h>
#include <immintrin.h>
// Cantidad de segundos en un dia: 86400
#define VALORES_POR_SENSOR 86400

// Cantidad de sensores de humedad del suelo
#define CANT_SENSORES_HUM_SUELO 2000
// Seed de generación de numeros pseudo-aleatorios
#define SEED 1u

// Para los datos de los sensores usamos double (8 bytes)
// Para los resultados de los sensores usamos double (8 bytes)
// Para el contador de resultados correctos e incorrectos usamos int (4 bytes)
// Utilizaremos registros de 256 bits => __m256d

// TODO Restar la cantidad de correctos a la cantidad de segundos por dia para obtener los incorrectos

int main()
{

    // Declaracion de vectores iniciales
    double *vec_tempAire, *vec_humAire;
    double **vec_vec_humSuelo;
    // Declaracion de vectores resultado
    double *vec_res_tempAire, *vec_res_humAire, *vec_res_humSuelo;
    // Declaracion de vectores para cargar los registros comparativos
    double *vec_condiciones;

    // Declaracion de variables
    int i, x, cantElementosPorRegistro, cantIteracionesPorRegistro;
    int type_sensor_size;
    double total_tempAire, total_humAire, total_humSuelo;
    double rand_num, acum;

    type_sensor_size = sizeof(double);                                          // 8
    cantElementosPorRegistro = sizeof(__m256d) / type_sensor_size;              // 4
    cantIteracionesPorRegistro = VALORES_POR_SENSOR / cantElementosPorRegistro; // 21600

    // Seed del rand
    srand(SEED);

    //! COMIENZO INICIALIZACION

    //* Vectores de los valores de sensores de humedad del suelo

    rand_num = 90.0;
    if (posix_memalign((void **)&vec_tempAire, 32, VALORES_POR_SENSOR * type_sensor_size) != 0)
    {
        return 1;
    }

    for (x = 0; x < VALORES_POR_SENSOR; x++)
    { // TODO Loop Unrolling
        // Llenamos los vectores con valores random entre -40 y 50
        vec_tempAire[x] = (double)rand() / ((double)RAND_MAX) * rand_num - 40;
    }

    rand_num = 100.0;
    if (posix_memalign((void **)&vec_humAire, 32, VALORES_POR_SENSOR * type_sensor_size) != 0)
    {
        return 1;
    }
    for (x = 0; x < VALORES_POR_SENSOR; x++)
    { // TODO Loop Unrolling
        // Llenamos los vectores con valores random entre 0 y 100
        vec_humAire[x] = (double)rand() / ((double)RAND_MAX) * rand_num;
    }

    //vec_vec_humSuelo = malloc(CANT_SENSORES_HUM_SUELO * type_sensor_size);
    if (posix_memalign((void **)&vec_vec_humSuelo, 32, CANT_SENSORES_HUM_SUELO * sizeof(double *)) != 0)
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
            vec_vec_humSuelo[i][x] = (double)rand() / ((double)RAND_MAX) * rand_num;
        }
    }

    //* Vectores de Resultado
    if (posix_memalign((void **)&vec_res_humAire, 32, cantElementosPorRegistro * type_sensor_size) != 0)
    {
        return 1;
    }

    if (posix_memalign((void **)&vec_res_tempAire, 32, cantElementosPorRegistro * type_sensor_size) != 0)
    {
        return 1;
    }

    if (posix_memalign((void **)&vec_res_humSuelo, 32, cantElementosPorRegistro * type_sensor_size) != 0)
    {
        return 1;
    }

    //! FIN INICIALIZACION

    // Declaracion de registros
    __m256d reg_comp_a, reg_comp_b, reg_comp_c, reg_actual, reg_actual_temporal, reg_acum;

    //* Humedad del Aire
    // Creamos el valor del registro a ser usado para comparar
    // Tenemos 3 elementos: 75%, 1, 0 - cargados 4 veces en el registro
    if (posix_memalign((void **)&vec_condiciones, 32, cantElementosPorRegistro * type_sensor_size * 3) != 0)
    {
        return 1;
    }
    for (i = 0; i < cantElementosPorRegistro; i++)
    {
        vec_condiciones[i] = 75.0;
    }
    x = cantElementosPorRegistro * 2;
    for (i = cantElementosPorRegistro; i < x; i++)
    {
        vec_condiciones[i] = 1.0;
    }
    x = cantElementosPorRegistro * 3;
    for (i = cantElementosPorRegistro * 2; i < x; i++)
    {
        vec_condiciones[i] = 0.0;
    }

    // Cargar el valor de los 4 elementos en el registro

    reg_comp_a = _mm256_load_pd((double const *)vec_condiciones);                                // 75
    reg_comp_b = _mm256_load_pd((double const *)vec_condiciones + (cantElementosPorRegistro));   // 1
    reg_acum = _mm256_load_pd((double const *)vec_condiciones + (cantElementosPorRegistro * 2)); // 0

    for (i = 0; i < cantIteracionesPorRegistro; i++)
    {
        reg_actual = _mm256_load_pd((double const *)vec_humAire + (i * cantElementosPorRegistro));
        reg_actual = _mm256_cmp_pd(reg_actual, reg_comp_a, 2); // 2 => Less or Equal
        reg_actual = _mm256_and_pd(reg_actual, reg_comp_b);    // AND con el valor 1
        reg_acum = _mm256_add_pd(reg_actual, reg_acum);
    }

    _mm256_store_pd((double *)vec_res_humAire, reg_acum);

    acum = 0.0;
    for (i = 0; i < cantElementosPorRegistro; i++)
    {
        acum += vec_res_humAire[i];
    }
    total_humAire = acum;

    free(vec_condiciones);

    //* Temperatura del Aire
    // Creamos el valor del registro a ser usado para comparar
    // Tenemos 4 valores: 20º, 30º, 1, 0 - cargados 4 veces en el registro.
    if (posix_memalign((void **)&vec_condiciones, 32, cantElementosPorRegistro * type_sensor_size * 4) != 0)
    {
        return 1;
    }
    for (i = 0; i < cantElementosPorRegistro; i++)
    {
        vec_condiciones[i] = 30.0;
    }
    x = cantElementosPorRegistro * 2;
    for (i = cantElementosPorRegistro; i < x; i++)
    {
        vec_condiciones[i] = 20.0;
    }
    x = cantElementosPorRegistro * 3;
    for (i = cantElementosPorRegistro * 2; i < x; i++)
    {
        vec_condiciones[i] = 1.0;
    }
    x = cantElementosPorRegistro * 4;
    for (i = cantElementosPorRegistro * 3; i < x; i++)
    {
        vec_condiciones[i] = 0.0;
    }

    // Cargar el valor de los 4 elementos en el registro
    reg_comp_a = _mm256_load_pd((double const *)vec_condiciones);                                  // 30
    reg_comp_b = _mm256_load_pd((double const *)vec_condiciones + (cantElementosPorRegistro));     // 20
    reg_comp_c = _mm256_load_pd((double const *)vec_condiciones + (cantElementosPorRegistro * 2)); // 1
    reg_acum = _mm256_load_pd((double const *)vec_condiciones + (cantElementosPorRegistro * 3));   // 0

    for (i = 0; i < cantIteracionesPorRegistro; i++)
    {
        reg_actual = _mm256_load_pd((double const *)vec_tempAire + (i * cantElementosPorRegistro));
        reg_actual_temporal = _mm256_cmp_pd(reg_actual, reg_comp_a, 2); // 2 => Less or Equal
        reg_actual = _mm256_cmp_pd(reg_actual, reg_comp_b, 13);         // 13 => Greater than or Equal
        reg_actual = _mm256_and_pd(reg_actual, reg_actual_temporal);    // Aqui obtenemos solo los que dieron =/= 0 en ambas comparaciones
        reg_actual = _mm256_and_pd(reg_actual, reg_comp_c);             // AND con el valor 1
        reg_acum = _mm256_add_pd(reg_actual, reg_acum);
    }

    _mm256_store_pd((double *)vec_res_tempAire, reg_acum);

    acum = 0.0;
    for (i = 0; i < cantElementosPorRegistro; i++)
    {
        acum += vec_res_tempAire[i];
    }
    total_tempAire = acum;

    free(vec_condiciones);

    //* Humedad del Suelo
    // Creamos el valor del registro a ser usado para comparar
    // Tenemos 3 valores: 30%, 1, 0 - cargados 4 veces en el registro.
    if (posix_memalign((void **)&vec_condiciones, 32, cantElementosPorRegistro * type_sensor_size * 3) != 0)
    {
        return 1;
    }
    for (i = 0; i < cantElementosPorRegistro; i++)
    {
        vec_condiciones[i] = 30.0;
    }
    x = cantElementosPorRegistro * 2;
    for (i = cantElementosPorRegistro; i < x; i++)
    {
        vec_condiciones[i] = 1.0;
    }
    x = cantElementosPorRegistro * 3;
    for (i = cantElementosPorRegistro * 2; i < x; i++)
    {
        vec_condiciones[i] = 0.0;
    }

    // Cargar el valor de los 4 elementos en el registro
    reg_comp_a = _mm256_load_pd((double const *)vec_condiciones);                                // 30
    reg_comp_b = _mm256_load_pd((double const *)vec_condiciones + (cantElementosPorRegistro));   // 1
    reg_acum = _mm256_load_pd((double const *)vec_condiciones + (cantElementosPorRegistro * 2)); // 0

    for (i = 0; i < CANT_SENSORES_HUM_SUELO; i++)
    {
        for (x = 0; x < cantIteracionesPorRegistro; x++)
        {
            reg_actual = _mm256_load_pd((double const *)vec_vec_humSuelo[i] + (x * cantElementosPorRegistro));
            reg_actual = _mm256_cmp_pd(reg_actual, reg_comp_a, 13); // 13 => Greater than or Equal
            reg_actual = _mm256_and_pd(reg_actual, reg_comp_b);     // AND con el valor 1
            reg_acum = _mm256_add_pd(reg_actual, reg_acum);
        }
    }

    _mm256_store_pd((double *)vec_res_humSuelo, reg_acum);

    acum = 0.0;
    for (i = 0; i < cantElementosPorRegistro; i++)
    {
        acum += vec_res_humSuelo[i];
    }
    total_humSuelo = acum;

    free(vec_condiciones);

    // Imprimimos los resultados :)
    printf("Temperatura del Aire: [C] %lf [IC] %lf \n", total_tempAire, VALORES_POR_SENSOR - total_tempAire);
    printf("Humedad del Aire: [C] %lf [IC] %lf \n", total_humAire, VALORES_POR_SENSOR - total_humAire);
    printf("Humedad del Suelo: [C] %lf [IC] %lf \n", total_humSuelo, (VALORES_POR_SENSOR * CANT_SENSORES_HUM_SUELO) - total_humSuelo);

    return 0;
}