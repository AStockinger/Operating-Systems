## One Time Pad (OTP) is written to run on Linux.

## To Compile:
type `./compileall`


## To Execute:
#### Set up the Daemons:
`otp_enc_d ENCRYPT_PORT &` and `otp_dec_d DECRYPT_PORT &` where the PORT numbers are different for each. These should be running in the background to wait for encryption/decryption requests.

#### Generate a key file
`keygen SIZEOFKEY > FILENAME` where SIZEOFKEY is the number of characters, and filename is where the text should be outputted to.

#### Encrypt plaintext
`otp_enc plaintextfile keyfile ENCRYPT_PORT > cipherfile` where the cipher file is the output that will contain a ciphered version of the plaintext file.

#### Decrypt cipher
`otp_dec cipherfile keyfile DECRYPT_PORT > plaintextfile` where the plaintext file is the output file that will contain the decrypted, original plaintext.


#### To run the test script:
`./p4script PORT1 PORT2 > results.txt 2>&1`


## About the Program
**Plaintext** is the term for the information that you wish to encrypt and protect. It is human readable.

**Ciphertext** is the term for the plaintext after it has been encrypted by your programs. Ciphertext is not human-readable, and in fact cannot be cracked, if the OTP system is used correctly.

A **Key** is the random sequence of characters that will be used to convert Plaintext to Ciphertext, and back again. *It must not be re-used, or else the encryption is in danger of being broken.*


“Suppose Alice wishes to send the message "HELLO" to Bob. Assume two pads of paper containing identical random sequences of letters were somehow previously produced and securely issued to both. Alice chooses the appropriate unused page from the pad. The way to do this is normally arranged for in advance, as for instance 'use the 12th sheet on 1 May', or 'use the next available sheet for the next message'.

The material on the selected sheet is the key for this message. Each letter from the pad will be combined in a predetermined way with one letter of the message. (It is common, but not required, to assign each letter a numerical value, e.g., "A" is 0, "B" is 1, and so on.)

In this example, the technique is to combine the key and the message using modular addition. The numerical values of corresponding message and key letters are added together, modulo 26. So, if key material begins with "XMCKL" and the message is "HELLO", then the coding would be done as follows:

```
      H       E       L       L       O  message
   7 (H)   4 (E)  11 (L)  11 (L)  14 (O) message
+ 23 (X)  12 (M)   2 (C)  10 (K)  11 (L) key
= 30      16      13      21      25     message + key
=  4 (E)  16 (Q)  13 (N)  21 (V)  25 (Z) message + key (mod 26)
      E       Q       N       V       Z  → ciphertext
```

If a number is larger than 26, then the remainder, after subtraction of 26, is taken [as the result]. This simply means that if the computations "go past" Z, the sequence starts again at A.

The ciphertext to be sent to Bob is thus "EQNVZ". Bob uses the matching key page and the same process, but in reverse, to obtain the plaintext. Here the key is subtracted from the ciphertext, again using modular arithmetic:

```
       E       Q       N       V       Z  ciphertext
    4 (E)  16 (Q)  13 (N)  21 (V)  25 (Z) ciphertext
-  23 (X)  12 (M)   2 (C)  10 (K)  11 (L) key
= -19       4      11      11      14     ciphertext – key
=   7 (H)   4 (E)  11 (L)  11 (L)  14 (O) ciphertext – key (mod 26)
       H       E       L       L       O  → message
```

Similar to the above, if a number is negative then 26 is added to make the number zero or higher.

Thus Bob recovers Alice's plaintext, the message "HELLO". Both Alice and Bob destroy the key sheet immediately after use, thus preventing reuse and an attack against the cipher.”