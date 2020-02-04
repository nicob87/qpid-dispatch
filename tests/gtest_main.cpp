/*
 * =====================================================================================
 *
 *       Filename:  gtest_main.c
 *
 *    Description: gT 
 *
 *        Version:  1.0
 *        Created:  02/03/2020 09:38:47 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <gtest/gtest.h>


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
