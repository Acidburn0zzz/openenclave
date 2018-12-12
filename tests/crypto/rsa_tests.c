// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#if defined(OE_BUILD_ENCLAVE)
#include <openenclave/enclave.h>
#endif

#include <openenclave/internal/cert.h>
#include <openenclave/internal/rsa.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "readfile.h"
#include "tests.h"

/* _CERT1 use as a Leaf cert
 * CHAIN1 consists Intermediate & Root cert.
 * CHAIN2 consists Intermediate2 & Root2 cert.
 * MIXED_CHAIN is a concatenation of two unrelated chains: CHAIN1 and CHAIN2
 * _PRIVATE_KEY use as a Leaf.key.pem
 * _PUBLIC_KEY use as a Leaf_public.key.pem
 * _SIGNATURE generates by Leaf.key.pem
 */

static char _PRIVATE_KEY[max_key_size];
static char _PUBLIC_KEY[max_key_size];
static char _CERT1[max_cert_size];
static char CHAIN1[max_cert_chain_size];
static char CHAIN2[max_cert_chain_size];
static char MIXED_CHAIN[max_cert_chain_size * 2];
static uint8_t _CERT1_RSA_MODULUS[max_cert_size];
static uint8_t _SIGNATURE[max_sign_size];
/* RSA exponent of CERT */
static const char _CERT_RSA_EXPONENT[] = {0x01, 0x00, 0x01};

size_t rsa_modules_size;
size_t sign_size;

// Test RSA signing operation over an ASCII alphabet string.
static void _test_sign()
{
    printf("=== begin %s()\n", __FUNCTION__);

    oe_result_t r;
    oe_rsa_private_key_t key = {0};
    uint8_t* signature = NULL;
    size_t signature_size = 0;

    r = oe_rsa_private_key_read_pem(
        &key, (const uint8_t*)_PRIVATE_KEY, strlen(_PRIVATE_KEY) + 1);
    OE_TEST(r == OE_OK);

    r = oe_rsa_private_key_sign(
        &key,
        OE_HASH_TYPE_SHA256,
        &ALPHABET_HASH,
        sizeof(ALPHABET_HASH),
        signature,
        &signature_size);
    OE_TEST(r == OE_BUFFER_TOO_SMALL);

    OE_TEST(signature = (uint8_t*)malloc(signature_size));

    r = oe_rsa_private_key_sign(
        &key,
        OE_HASH_TYPE_SHA256,
        &ALPHABET_HASH,
        sizeof(ALPHABET_HASH),
        signature,
        &signature_size);
    OE_TEST(r == OE_OK);

    OE_TEST(signature_size == sign_size);

    OE_TEST(memcmp(signature, &_SIGNATURE, sign_size) == 0);

    oe_rsa_private_key_free(&key);
    free(signature);

    printf("=== passed %s()\n", __FUNCTION__);
}

// Test RSA verify operation over an ASCII alphabet string.
static void _test_verify()
{
    printf("=== begin %s()\n", __FUNCTION__);

    oe_result_t r;
    oe_rsa_public_key_t key = {0};

    r = oe_rsa_public_key_read_pem(
        &key, (const uint8_t*)_PUBLIC_KEY, strlen(_PUBLIC_KEY) + 1);
    OE_TEST(r == OE_OK);

    r = oe_rsa_public_key_verify(
        &key,
        OE_HASH_TYPE_SHA256,
        &ALPHABET_HASH,
        sizeof(ALPHABET_HASH),
        _SIGNATURE,
        sign_size);
    OE_TEST(r == OE_OK);

    oe_rsa_public_key_free(&key);

    printf("=== passed %s()\n", __FUNCTION__);
}

static void _test_cert_verify_good()
{
    printf("=== begin %s()\n", __FUNCTION__);

    oe_result_t r;
    oe_verify_cert_error_t error = {0};
    oe_cert_t cert = {0};
    oe_cert_chain_t chain = {0};

    r = oe_cert_read_pem(&cert, _CERT1, strlen(_CERT1) + 1);
    OE_TEST(r == OE_OK);

    r = oe_cert_chain_read_pem(&chain, CHAIN1, strlen(CHAIN1) + 1);
    OE_TEST(r == OE_OK);

    r = oe_cert_verify(&cert, &chain, NULL, 0, &error);
    OE_TEST(r == OE_OK);

    oe_cert_free(&cert);
    oe_cert_chain_free(&chain);

    printf("=== passed %s()\n", __FUNCTION__);
}

static void _test_cert_verify_bad()
{
    printf("=== begin %s()\n", __FUNCTION__);

    oe_result_t r;
    oe_verify_cert_error_t error = {0};
    oe_cert_t cert = {0};
    oe_cert_chain_t chain = {0};

    r = oe_cert_read_pem(&cert, _CERT1, strlen(_CERT1) + 1);
    OE_TEST(r == OE_OK);

    /* Chain does not contain a root for this certificate */
    r = oe_cert_chain_read_pem(&chain, CHAIN2, strlen(CHAIN2) + 1);
    OE_TEST(r == OE_OK);

    r = oe_cert_verify(&cert, &chain, NULL, 0, &error);
    OE_TEST(r == OE_VERIFY_FAILED);

    oe_cert_free(&cert);
    oe_cert_chain_free(&chain);

    printf("=== passed %s()\n", __FUNCTION__);
}

static void _test_mixed_chain()
{
    printf("=== begin %s()\n", __FUNCTION__);

    oe_result_t r;
    oe_cert_t cert = {0};
    oe_cert_chain_t chain = {0};

    r = oe_cert_read_pem(&cert, _CERT1, strlen(_CERT1) + 1);
    OE_TEST(r == OE_OK);

    /* Chain does not contain a root for this certificate */
    r = oe_cert_chain_read_pem(&chain, MIXED_CHAIN, strlen(MIXED_CHAIN) + 1);
    OE_TEST(r == OE_FAILURE);

    oe_cert_free(&cert);
    oe_cert_chain_free(&chain);

    printf("=== passed %s()\n", __FUNCTION__);
}

static void _test_generate()
{
    printf("=== begin %s()\n", __FUNCTION__);

    oe_result_t r;
    oe_rsa_private_key_t private_key = {0};
    oe_rsa_public_key_t public_key = {0};
    uint8_t* signature = NULL;
    size_t signature_size = 0;

    r = oe_rsa_generate_key_pair(1024, 3, &private_key, &public_key);
    OE_TEST(r == OE_OK);

    r = oe_rsa_private_key_sign(
        &private_key,
        OE_HASH_TYPE_SHA256,
        &ALPHABET_HASH,
        sizeof(ALPHABET_HASH),
        signature,
        &signature_size);
    OE_TEST(r == OE_BUFFER_TOO_SMALL);

    OE_TEST(signature = (uint8_t*)malloc(signature_size));

    r = oe_rsa_private_key_sign(
        &private_key,
        OE_HASH_TYPE_SHA256,
        &ALPHABET_HASH,
        sizeof(ALPHABET_HASH),
        signature,
        &signature_size);
    OE_TEST(r == OE_OK);

    r = oe_rsa_public_key_verify(
        &public_key,
        OE_HASH_TYPE_SHA256,
        &ALPHABET_HASH,
        sizeof(ALPHABET_HASH),
        signature,
        signature_size);
    OE_TEST(r == OE_OK);

    free(signature);
    oe_rsa_private_key_free(&private_key);
    oe_rsa_public_key_free(&public_key);

    printf("=== passed %s()\n", __FUNCTION__);
}

static void _test_write_private()
{
    printf("=== begin %s()\n", __FUNCTION__);

    oe_result_t r;
    oe_rsa_private_key_t key = {0};
    void* pem_data = NULL;
    size_t pem_size = 0;

    r = oe_rsa_private_key_read_pem(
        &key, (const uint8_t*)_PRIVATE_KEY, strlen(_PRIVATE_KEY) + 1);
    OE_TEST(r == OE_OK);

    r = oe_rsa_private_key_write_pem(&key, pem_data, &pem_size);
    OE_TEST(r == OE_BUFFER_TOO_SMALL);

    OE_TEST(pem_data = (uint8_t*)malloc(pem_size));

    r = oe_rsa_private_key_write_pem(&key, pem_data, &pem_size);
    OE_TEST(r == OE_OK);

    OE_TEST((strlen(_PRIVATE_KEY) + 1) == pem_size);
    OE_TEST(memcmp(_PRIVATE_KEY, pem_data, pem_size) == 0);

    free(pem_data);
    oe_rsa_private_key_free(&key);

    printf("=== passed %s()\n", __FUNCTION__);
}

static void _test_write_public()
{
    printf("=== begin %s()\n", __FUNCTION__);

    oe_result_t r;
    oe_rsa_public_key_t key = {0};
    void* pem_data = NULL;
    size_t pem_size = 0;

    r = oe_rsa_public_key_read_pem(
        &key, (const uint8_t*)_PUBLIC_KEY, strlen(_PUBLIC_KEY) + 1);
    OE_TEST(r == OE_OK);

    r = oe_rsa_public_key_write_pem(&key, pem_data, &pem_size);
    OE_TEST(r == OE_BUFFER_TOO_SMALL);

    OE_TEST(pem_data = (uint8_t*)malloc(pem_size));

    r = oe_rsa_public_key_write_pem(&key, pem_data, &pem_size);
    OE_TEST(r == OE_OK);

    OE_TEST((strlen(_PUBLIC_KEY) + 1) == pem_size);
    OE_TEST(memcmp(_PUBLIC_KEY, pem_data, pem_size) == 0);

    free(pem_data);
    oe_rsa_public_key_free(&key);

    printf("=== passed %s()\n", __FUNCTION__);
}

static void _test_cert_methods()
{
    printf("=== begin %s()\n", __FUNCTION__);

    oe_result_t r;

    /* Test oe_cert_get_rsa_public_key() */
    {
        oe_cert_t cert = {0};

        r = oe_cert_read_pem(&cert, _CERT1, strlen(_CERT1) + 1);
        OE_TEST(r == OE_OK);

        oe_rsa_public_key_t key = {0};
        r = oe_cert_get_rsa_public_key(&cert, &key);
        OE_TEST(r == OE_OK);

        /* Test oe_rsa_public_key_get_modulus() */
        {
            uint8_t* data;
            size_t size = 0;

            /* Determine required buffer size */
            r = oe_rsa_public_key_get_modulus(&key, NULL, &size);
            OE_TEST(r == OE_BUFFER_TOO_SMALL);
            OE_TEST(size == rsa_modules_size);

            /* Fetch the key bytes */
            OE_TEST(data = (uint8_t*)malloc(size));
            r = oe_rsa_public_key_get_modulus(&key, data, &size);
            OE_TEST(r == OE_OK);

            /* Does it match expected modulus? */
            OE_TEST(size == rsa_modules_size);
            OE_TEST(memcmp(data, _CERT1_RSA_MODULUS, size) == 0);
            free(data);
        }

        /* Test oe_rsa_public_key_get_exponent() */
        {
            uint8_t* data;
            size_t size = 0;

            /* Determine required buffer size */
            r = oe_rsa_public_key_get_exponent(&key, NULL, &size);
            OE_TEST(r == OE_BUFFER_TOO_SMALL);
            OE_TEST(size == sizeof(_CERT_RSA_EXPONENT));

            /* Fetch the key bytes */
            OE_TEST(data = (uint8_t*)malloc(size));
            r = oe_rsa_public_key_get_exponent(&key, data, &size);
            OE_TEST(r == OE_OK);

            /* Does it match expected exponent */
            OE_TEST(size == sizeof(_CERT_RSA_EXPONENT));
            OE_TEST(memcmp(data, _CERT_RSA_EXPONENT, size) == 0);
            free(data);
        }

        /* Test oe_rsa_public_key_equal() */
        {
            bool equal;
            OE_TEST(oe_rsa_public_key_equal(&key, &key, &equal) == OE_OK);
            OE_TEST(equal == true);
        }

        oe_rsa_public_key_free(&key);
        oe_cert_free(&cert);
    }

    /* Test oe_cert_chain_get_cert() */
    {
        oe_cert_chain_t chain = {0};

        /* Load the chain from PEM format */
        r = oe_cert_chain_read_pem(&chain, CHAIN1, strlen(CHAIN1) + 1);
        OE_TEST(r == OE_OK);

        /* Get the length of the chain */
        size_t length;
        r = oe_cert_chain_get_length(&chain, &length);
        OE_TEST(r == OE_OK);
        OE_TEST(length == 2);

        /* Get each certificate in the chain */
        for (size_t i = 0; i < length; i++)
        {
            oe_cert_t cert = {0};
            r = oe_cert_chain_get_cert(&chain, i, &cert);
            OE_TEST(r == OE_OK);
            oe_cert_free(&cert);
        }

        /* Test out of bounds */
        {
            oe_cert_t cert = {0};
            r = oe_cert_chain_get_cert(&chain, length + 1, &cert);
            OE_TEST(r == OE_OUT_OF_BOUNDS);
            oe_cert_free(&cert);
        }

        oe_cert_chain_free(&chain);
    }

    /* Test oe_cert_chain_get_root_cert() and oe_cert_chain_get_leaf_cert() */
    {
        oe_cert_chain_t chain = {0};
        oe_cert_t root = {0};
        oe_cert_t leaf = {0};

        /* Load the chain from PEM format */
        r = oe_cert_chain_read_pem(&chain, CHAIN1, strlen(CHAIN1) + 1);
        OE_TEST(r == OE_OK);

        /* Get the root certificate */
        r = oe_cert_chain_get_root_cert(&chain, &root);
        OE_TEST(r == OE_OK);

        /* Get the leaf certificate */
        r = oe_cert_chain_get_leaf_cert(&chain, &leaf);
        OE_TEST(r == OE_OK);

        /* Check that the keys are identical for top and root certificate */
        {
            oe_rsa_public_key_t root_key = {0};
            oe_rsa_public_key_t cert_key = {0};

            OE_TEST(oe_cert_get_rsa_public_key(&root, &root_key) == OE_OK);

            oe_rsa_public_key_free(&root_key);
            oe_rsa_public_key_free(&cert_key);
        }

        /* Check that the keys are not identical for leaf and root */
        {
            oe_rsa_public_key_t root_key = {0};
            oe_rsa_public_key_t leaf_key = {0};
            bool equal;

            OE_TEST(oe_cert_get_rsa_public_key(&root, &root_key) == OE_OK);
            OE_TEST(oe_cert_get_rsa_public_key(&leaf, &leaf_key) == OE_OK);

            OE_TEST(
                oe_rsa_public_key_equal(&root_key, &leaf_key, &equal) == OE_OK);
            OE_TEST(equal == false);

            oe_rsa_public_key_free(&root_key);
            oe_rsa_public_key_free(&leaf_key);
        }

        oe_cert_free(&root);
        oe_cert_free(&leaf);
        oe_cert_chain_free(&chain);
    }

    printf("=== passed %s()\n", __FUNCTION__);
}

void TestRSA(void)
{
    OE_TEST(read_cert("../data/Leaf.crt.pem", _CERT1) == OE_OK);
    OE_TEST(
        read_chain(
            "../data/Intermediate.crt.pem", "../data/RootCA.crt.pem", CHAIN1) ==
        OE_OK);
    OE_TEST(
        read_chain(
            "../data/Intermediate2.crt.pem",
            "../data/RootCA2.crt.pem",
            CHAIN2) == OE_OK);
    OE_TEST(
        read_mod(
            "../data/Leaf_modules.bin",
            _CERT1_RSA_MODULUS,
            &rsa_modules_size) == OE_OK);
    OE_TEST(read_key("../data/Leaf.key.pem", _PRIVATE_KEY) == OE_OK);
    OE_TEST(read_key("../data/Leaf_public.key.pem", _PUBLIC_KEY) == OE_OK);
    OE_TEST(
        read_sign("../data/test_rsa_signature", _SIGNATURE, &sign_size) ==
        OE_OK);
    OE_TEST(read_mixed_chain(MIXED_CHAIN, CHAIN1, CHAIN2) == OE_OK);
    _test_cert_methods();
    _test_cert_verify_good();
    _test_cert_verify_bad();
    _test_mixed_chain();
    _test_generate();
    _test_sign();
    _test_verify();
    _test_write_private();
    _test_write_public();
}
