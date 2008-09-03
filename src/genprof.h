#ifndef __GENPROF_H__
#define __GENPROF_H__

/*
 Copyright (c) 2006 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*!
 \file    genprof.h
 \author  Trevor Williams  (phase1geo@gmail.com)
 \date    12/10/2007
*/

#include "defines.h"

#define NUM_PROFILES 985

#ifdef DEBUG
#define UNREGISTERED 0
#define ARC_FIND_FROM_STATE 1
#define ARC_FIND_TO_STATE 2
#define ARC_FIND_ARC 3
#define ARC_FIND_ARC_BY_EXCLUSION_ID 4
#define ARC_CREATE 5
#define ARC_ADD 6
#define ARC_STATE_HITS 7
#define ARC_TRANSITION_HITS 8
#define ARC_TRANSITION_EXCLUDED 9
#define ARC_GET_STATS 10
#define ARC_DB_WRITE 11
#define ARC_DB_READ 12
#define ARC_DB_MERGE 13
#define ARC_MERGE 14
#define ARC_GET_STATES 15
#define ARC_GET_TRANSITIONS 16
#define ARC_ARE_ANY_EXCLUDED 17
#define ARC_DEALLOC 18
#define ASSERTION_PARSE 19
#define ASSERTION_PARSE_ATTR 20
#define ASSERTION_GET_STATS 21
#define ASSERTION_DISPLAY_INSTANCE_SUMMARY 22
#define ASSERTION_INSTANCE_SUMMARY 23
#define ASSERTION_DISPLAY_FUNIT_SUMMARY 24
#define ASSERTION_FUNIT_SUMMARY 25
#define ASSERTION_DISPLAY_VERBOSE 26
#define ASSERTION_INSTANCE_VERBOSE 27
#define ASSERTION_FUNIT_VERBOSE 28
#define ASSERTION_REPORT 29
#define ASSERTION_GET_FUNIT_SUMMARY 30
#define ASSERTION_COLLECT 31
#define ASSERTION_GET_COVERAGE 32
#define ATTRIBUTE_CREATE 33
#define ATTRIBUTE_PARSE 34
#define ATTRIBUTE_DEALLOC 35
#define BIND_ADD 36
#define BIND_APPEND_FSM_EXPR 37
#define BIND_REMOVE 38
#define BIND_FIND_SIG_NAME 39
#define BIND_PARAM 40
#define BIND_SIGNAL 41
#define BIND_TASK_FUNCTION_PORTS 42
#define BIND_TASK_FUNCTION_NAMEDBLOCK 43
#define BIND_PERFORM 44
#define BIND_DEALLOC 45
#define CODEGEN_CREATE_EXPR_HELPER 46
#define CODEGEN_CREATE_EXPR 47
#define CODEGEN_GEN_EXPR 48
#define COMBINATION_CALC_DEPTH 49
#define COMBINATION_DOES_MULTI_EXP_NEED_UL 50
#define COMBINATION_MULTI_EXPR_CALC 51
#define COMBINATION_IS_EXPR_MULTI_NODE 52
#define COMBINATION_GET_TREE_STATS 53
#define COMBINATION_RESET_COUNTED_EXPRS 54
#define COMBINATION_RESET_COUNTED_EXPR_TREE 55
#define COMBINATION_GET_STATS 56
#define COMBINATION_GET_FUNIT_SUMMARY 57
#define COMBINATION_GET_INST_SUMMARY 58
#define COMBINATION_DISPLAY_INSTANCE_SUMMARY 59
#define COMBINATION_INSTANCE_SUMMARY 60
#define COMBINATION_DISPLAY_FUNIT_SUMMARY 61
#define COMBINATION_FUNIT_SUMMARY 62
#define COMBINATION_DRAW_LINE 63
#define COMBINATION_DRAW_CENTERED_LINE 64
#define COMBINATION_UNDERLINE_TREE 65
#define COMBINATION_PREP_LINE 66
#define COMBINATION_UNDERLINE 67
#define COMBINATION_UNARY 68
#define COMBINATION_EVENT 69
#define COMBINATION_TWO_VARS 70
#define COMBINATION_MULTI_VAR_EXPRS 71
#define COMBINATION_MULTI_EXPR_OUTPUT_LENGTH 72
#define COMBINATION_MULTI_EXPR_OUTPUT 73
#define COMBINATION_MULTI_VARS 74
#define COMBINATION_GET_MISSED_EXPR 75
#define COMBINATION_LIST_MISSED 76
#define COMBINATION_OUTPUT_EXPR 77
#define COMBINATION_DISPLAY_VERBOSE 78
#define COMBINATION_INSTANCE_VERBOSE 79
#define COMBINATION_FUNIT_VERBOSE 80
#define COMBINATION_COLLECT 81
#define COMBINATION_GET_EXCLUDE_LIST 82
#define COMBINATION_GET_EXPRESSION 83
#define COMBINATION_GET_COVERAGE 84
#define COMBINATION_REPORT 85
#define DB_CREATE 86
#define DB_CLOSE 87
#define DB_CHECK_FOR_TOP_MODULE 88
#define DB_WRITE 89
#define DB_READ 90
#define DB_MERGE_FUNITS 91
#define DB_SCALE_TO_PRECISION 92
#define DB_CREATE_UNNAMED_SCOPE 93
#define DB_IS_UNNAMED_SCOPE 94
#define DB_SET_TIMESCALE 95
#define DB_GET_CURR_FUNIT 96
#define DB_GET_EXCLUSION_ID_SIZE 97
#define DB_GEN_EXCLUSION_ID 98
#define DB_ADD_INSTANCE 99
#define DB_ADD_MODULE 100
#define DB_END_MODULE 101
#define DB_ADD_FUNCTION_TASK_NAMEDBLOCK 102
#define DB_END_FUNCTION_TASK_NAMEDBLOCK 103
#define DB_ADD_DECLARED_PARAM 104
#define DB_ADD_OVERRIDE_PARAM 105
#define DB_ADD_VECTOR_PARAM 106
#define DB_ADD_DEFPARAM 107
#define DB_ADD_SIGNAL 108
#define DB_ADD_ENUM 109
#define DB_END_ENUM_LIST 110
#define DB_ADD_TYPEDEF 111
#define DB_FIND_SIGNAL 112
#define DB_ADD_GEN_ITEM_BLOCK 113
#define DB_FIND_GEN_ITEM 114
#define DB_FIND_TYPEDEF 115
#define DB_GET_CURR_GEN_BLOCK 116
#define DB_CURR_SIGNAL_COUNT 117
#define DB_CREATE_EXPRESSION 118
#define DB_BIND_EXPR_TREE 119
#define DB_CREATE_EXPR_FROM_STATIC 120
#define DB_ADD_EXPRESSION 121
#define DB_CREATE_SENSITIVITY_LIST 122
#define DB_PARALLELIZE_STATEMENT 123
#define DB_CREATE_STATEMENT 124
#define DB_ADD_STATEMENT 125
#define DB_REMOVE_STATEMENT_FROM_CURRENT_FUNIT 126
#define DB_REMOVE_STATEMENT 127
#define DB_CONNECT_STATEMENT_TRUE 128
#define DB_CONNECT_STATEMENT_FALSE 129
#define DB_GEN_ITEM_CONNECT_TRUE 130
#define DB_GEN_ITEM_CONNECT_FALSE 131
#define DB_GEN_ITEM_CONNECT 132
#define DB_STATEMENT_CONNECT 133
#define DB_CREATE_ATTR_PARAM 134
#define DB_PARSE_ATTRIBUTE 135
#define DB_REMOVE_STMT_BLKS_CALLING_STATEMENT 136
#define DB_GEN_CURR_INST_SCOPE 137
#define DB_SYNC_CURR_INSTANCE 138
#define DB_SET_VCD_SCOPE 139
#define DB_VCD_UPSCOPE 140
#define DB_ASSIGN_SYMBOL 141
#define DB_SET_SYMBOL_CHAR 142
#define DB_SET_SYMBOL_STRING 143
#define DB_DO_TIMESTEP 144
#define ENUMERATE_ADD_ITEM 145
#define ENUMERATE_END_LIST 146
#define ENUMERATE_RESOLVE 147
#define ENUMERATE_DEALLOC 148
#define ENUMERATE_DEALLOC_LIST 149
#define EXCLUDE_EXPR_ASSIGN_AND_RECALC 150
#define EXCLUDE_SIG_ASSIGN_AND_RECALC 151
#define EXCLUDE_ARC_ASSIGN_AND_RECALC 152
#define EXCLUDE_IS_LINE_EXCLUDED 153
#define EXCLUDE_SET_LINE_EXCLUDE 154
#define EXCLUDE_IS_TOGGLE_EXCLUDED 155
#define EXCLUDE_SET_TOGGLE_EXCLUDE 156
#define EXCLUDE_IS_COMB_EXCLUDED 157
#define EXCLUDE_SET_COMB_EXCLUDE 158
#define EXCLUDE_IS_FSM_EXCLUDED 159
#define EXCLUDE_SET_FSM_EXCLUDE 160
#define EXCLUDE_IS_ASSERT_EXCLUDED 161
#define EXCLUDE_SET_ASSERT_EXCLUDE 162
#define EXCLUDE_FIND_EXCLUDE_REASON 163
#define EXCLUDE_DB_WRITE 164
#define EXCLUDE_DB_READ 165
#define EXCLUDE_FIND_SIGNAL 166
#define EXCLUDE_FIND_EXPRESSION 167
#define EXCLUDE_FIND_FSM_ARC 168
#define EXCLUDED_GET_MESSAGE 169
#define EXCLUDE_HANDLE_EXCLUDE_REASON 170
#define EXCLUDE_LINE_FROM_ID 171
#define EXCLUDE_TOGGLE_FROM_ID 172
#define EXCLUDE_MEMORY_FROM_ID 173
#define EXCLUDE_EXPR_FROM_ID 174
#define EXCLUDE_FSM_FROM_ID 175
#define EXCLUDE_ASSERT_FROM_ID 176
#define EXCLUDE_APPLY_EXCLUSIONS 177
#define COMMAND_EXCLUDE 178
#define EXPRESSION_CREATE_TMP_VECS 179
#define EXPRESSION_CREATE_VALUE 180
#define EXPRESSION_CREATE 181
#define EXPRESSION_SET_VALUE 182
#define EXPRESSION_SET_SIGNED 183
#define EXPRESSION_RESIZE 184
#define EXPRESSION_GET_ID 185
#define EXPRESSION_GET_FIRST_LINE_EXPR 186
#define EXPRESSION_GET_LAST_LINE_EXPR 187
#define EXPRESSION_GET_CURR_DIMENSION 188
#define EXPRESSION_FIND_RHS_SIGS 189
#define EXPRESSION_FIND_PARAMS 190
#define EXPRESSION_FIND_ULINE_ID 191
#define EXPRESSION_FIND_EXPR 192
#define EXPRESSION_CONTAINS_EXPR_CALLING_STMT 193
#define EXPRESSION_GET_ROOT_STATEMENT 194
#define EXPRESSION_ASSIGN_EXPR_IDS 195
#define EXPRESSION_DB_WRITE 196
#define EXPRESSION_DB_WRITE_TREE 197
#define EXPRESSION_DB_READ 198
#define EXPRESSION_DB_MERGE 199
#define EXPRESSION_MERGE 200
#define EXPRESSION_STRING_OP 201
#define EXPRESSION_STRING 202
#define EXPRESSION_OP_FUNC__XOR 203
#define EXPRESSION_OP_FUNC__XOR_A 204
#define EXPRESSION_OP_FUNC__MULTIPLY 205
#define EXPRESSION_OP_FUNC__MULTIPLY_A 206
#define EXPRESSION_OP_FUNC__DIVIDE 207
#define EXPRESSION_OP_FUNC__DIVIDE_A 208
#define EXPRESSION_OP_FUNC__MOD 209
#define EXPRESSION_OP_FUNC__MOD_A 210
#define EXPRESSION_OP_FUNC__ADD 211
#define EXPRESSION_OP_FUNC__ADD_A 212
#define EXPRESSION_OP_FUNC__SUBTRACT 213
#define EXPRESSION_OP_FUNC__SUB_A 214
#define EXPRESSION_OP_FUNC__AND 215
#define EXPRESSION_OP_FUNC__AND_A 216
#define EXPRESSION_OP_FUNC__OR 217
#define EXPRESSION_OP_FUNC__OR_A 218
#define EXPRESSION_OP_FUNC__NAND 219
#define EXPRESSION_OP_FUNC__NOR 220
#define EXPRESSION_OP_FUNC__NXOR 221
#define EXPRESSION_OP_FUNC__LT 222
#define EXPRESSION_OP_FUNC__GT 223
#define EXPRESSION_OP_FUNC__LSHIFT 224
#define EXPRESSION_OP_FUNC__LSHIFT_A 225
#define EXPRESSION_OP_FUNC__RSHIFT 226
#define EXPRESSION_OP_FUNC__RSHIFT_A 227
#define EXPRESSION_OP_FUNC__ARSHIFT 228
#define EXPRESSION_OP_FUNC__ARSHIFT_A 229
#define EXPRESSION_OP_FUNC__EQ 230
#define EXPRESSION_OP_FUNC__CEQ 231
#define EXPRESSION_OP_FUNC__LE 232
#define EXPRESSION_OP_FUNC__GE 233
#define EXPRESSION_OP_FUNC__NE 234
#define EXPRESSION_OP_FUNC__CNE 235
#define EXPRESSION_OP_FUNC__LOR 236
#define EXPRESSION_OP_FUNC__LAND 237
#define EXPRESSION_OP_FUNC__COND 238
#define EXPRESSION_OP_FUNC__COND_SEL 239
#define EXPRESSION_OP_FUNC__UINV 240
#define EXPRESSION_OP_FUNC__UAND 241
#define EXPRESSION_OP_FUNC__UNOT 242
#define EXPRESSION_OP_FUNC__UOR 243
#define EXPRESSION_OP_FUNC__UXOR 244
#define EXPRESSION_OP_FUNC__UNAND 245
#define EXPRESSION_OP_FUNC__UNOR 246
#define EXPRESSION_OP_FUNC__UNXOR 247
#define EXPRESSION_OP_FUNC__NULL 248
#define EXPRESSION_OP_FUNC__SIG 249
#define EXPRESSION_OP_FUNC__SBIT 250
#define EXPRESSION_OP_FUNC__MBIT 251
#define EXPRESSION_OP_FUNC__EXPAND 252
#define EXPRESSION_OP_FUNC__LIST 253
#define EXPRESSION_OP_FUNC__CONCAT 254
#define EXPRESSION_OP_FUNC__PEDGE 255
#define EXPRESSION_OP_FUNC__NEDGE 256
#define EXPRESSION_OP_FUNC__AEDGE 257
#define EXPRESSION_OP_FUNC__EOR 258
#define EXPRESSION_OP_FUNC__SLIST 259
#define EXPRESSION_OP_FUNC__DELAY 260
#define EXPRESSION_OP_FUNC__TRIGGER 261
#define EXPRESSION_OP_FUNC__CASE 262
#define EXPRESSION_OP_FUNC__CASEX 263
#define EXPRESSION_OP_FUNC__CASEZ 264
#define EXPRESSION_OP_FUNC__DEFAULT 265
#define EXPRESSION_OP_FUNC__BASSIGN 266
#define EXPRESSION_OP_FUNC__FUNC_CALL 267
#define EXPRESSION_OP_FUNC__TASK_CALL 268
#define EXPRESSION_OP_FUNC__NB_CALL 269
#define EXPRESSION_OP_FUNC__FORK 270
#define EXPRESSION_OP_FUNC__JOIN 271
#define EXPRESSION_OP_FUNC__DISABLE 272
#define EXPRESSION_OP_FUNC__REPEAT 273
#define EXPRESSION_OP_FUNC__EXPONENT 274
#define EXPRESSION_OP_FUNC__PASSIGN 275
#define EXPRESSION_OP_FUNC__MBIT_POS 276
#define EXPRESSION_OP_FUNC__MBIT_NEG 277
#define EXPRESSION_OP_FUNC__NEGATE 278
#define EXPRESSION_OP_FUNC__IINC 279
#define EXPRESSION_OP_FUNC__PINC 280
#define EXPRESSION_OP_FUNC__IDEC 281
#define EXPRESSION_OP_FUNC__PDEC 282
#define EXPRESSION_OP_FUNC__DLY_ASSIGN 283
#define EXPRESSION_OP_FUNC__DLY_OP 284
#define EXPRESSION_OP_FUNC__REPEAT_DLY 285
#define EXPRESSION_OP_FUNC__DIM 286
#define EXPRESSION_OP_FUNC__WAIT 287
#define EXPRESSION_OP_FUNC__FINISH 288
#define EXPRESSION_OP_FUNC__STOP 289
#define EXPRESSION_OPERATE 290
#define EXPRESSION_OPERATE_RECURSIVELY 291
#define EXPRESSION_IS_STATIC_ONLY 292
#define EXPRESSION_IS_ASSIGNED 293
#define EXPRESSION_IS_BIT_SELECT 294
#define EXPRESSION_IS_LAST_SELECT 295
#define EXPRESSION_IS_IN_RASSIGN 296
#define EXPRESSION_SET_ASSIGNED 297
#define EXPRESSION_SET_CHANGED 298
#define EXPRESSION_ASSIGN 299
#define EXPRESSION_DEALLOC 300
#define FSM_CREATE 301
#define FSM_ADD_ARC 302
#define FSM_CREATE_TABLES 303
#define FSM_DB_WRITE 304
#define FSM_DB_READ 305
#define FSM_DB_MERGE 306
#define FSM_MERGE 307
#define FSM_TABLE_SET 308
#define FSM_GET_STATS 309
#define FSM_GET_FUNIT_SUMMARY 310
#define FSM_GET_INST_SUMMARY 311
#define FSM_GATHER_SIGNALS 312
#define FSM_COLLECT 313
#define FSM_GET_COVERAGE 314
#define FSM_DISPLAY_INSTANCE_SUMMARY 315
#define FSM_INSTANCE_SUMMARY 316
#define FSM_DISPLAY_FUNIT_SUMMARY 317
#define FSM_FUNIT_SUMMARY 318
#define FSM_DISPLAY_STATE_VERBOSE 319
#define FSM_DISPLAY_ARC_VERBOSE 320
#define FSM_DISPLAY_VERBOSE 321
#define FSM_INSTANCE_VERBOSE 322
#define FSM_FUNIT_VERBOSE 323
#define FSM_REPORT 324
#define FSM_DEALLOC 325
#define FSM_ARG_PARSE_STATE 326
#define FSM_ARG_PARSE 327
#define FSM_ARG_PARSE_VALUE 328
#define FSM_ARG_PARSE_TRANS 329
#define FSM_ARG_PARSE_ATTR 330
#define FSM_VAR_ADD 331
#define FSM_VAR_IS_OUTPUT_STATE 332
#define FSM_VAR_BIND_EXPR 333
#define FSM_VAR_ADD_EXPR 334
#define FSM_VAR_BIND_STMT 335
#define FSM_VAR_BIND_ADD 336
#define FSM_VAR_STMT_ADD 337
#define FSM_VAR_BIND 338
#define FSM_VAR_DEALLOC 339
#define FSM_VAR_REMOVE 340
#define FSM_VAR_CLEANUP 341
#define FUNC_ITER_DISPLAY 342
#define FUNC_ITER_SORT 343
#define FUNC_ITER_COUNT_STMT_ITERS 344
#define FUNC_ITER_ADD_STMT_ITERS 345
#define FUNC_ITER_ADD_SIG_LINKS 346
#define FUNC_ITER_INIT 347
#define FUNC_ITER_GET_NEXT_STATEMENT 348
#define FUNC_ITER_GET_NEXT_SIGNAL 349
#define FUNC_ITER_DEALLOC 350
#define FUNIT_INIT 351
#define FUNIT_CREATE 352
#define FUNIT_GET_CURR_MODULE 353
#define FUNIT_GET_CURR_MODULE_SAFE 354
#define FUNIT_GET_CURR_FUNCTION 355
#define FUNIT_GET_CURR_TASK 356
#define FUNIT_GET_PORT_COUNT 357
#define FUNIT_FIND_PARAM 358
#define FUNIT_FIND_SIGNAL 359
#define FUNIT_REMOVE_STMT_BLKS_CALLING_STMT 360
#define FUNIT_GEN_TASK_FUNCTION_NAMEDBLOCK_NAME 361
#define FUNIT_SIZE_ELEMENTS 362
#define FUNIT_DB_WRITE 363
#define FUNIT_DB_READ 364
#define FUNIT_DB_MERGE 365
#define FUNIT_MERGE 366
#define FUNIT_FLATTEN_NAME 367
#define FUNIT_FIND_BY_ID 368
#define FUNIT_IS_TOP_MODULE 369
#define FUNIT_IS_UNNAMED 370
#define FUNIT_IS_UNNAMED_CHILD_OF 371
#define FUNIT_IS_CHILD_OF 372
#define FUNIT_DISPLAY_SIGNALS 373
#define FUNIT_DISPLAY_EXPRESSIONS 374
#define STATEMENT_ADD_THREAD 375
#define FUNIT_PUSH_THREADS 376
#define STATEMENT_DELETE_THREAD 377
#define FUNIT_CLEAN 378
#define FUNIT_DEALLOC 379
#define GEN_ITEM_STRINGIFY 380
#define GEN_ITEM_DISPLAY 381
#define GEN_ITEM_DISPLAY_BLOCK_HELPER 382
#define GEN_ITEM_DISPLAY_BLOCK 383
#define GEN_ITEM_COMPARE 384
#define GEN_ITEM_FIND 385
#define GEN_ITEM_REMOVE_IF_CONTAINS_EXPR_CALLING_STMT 386
#define GEN_ITEM_GET_GENVAR 387
#define GEN_ITEM_VARNAME_CONTAINS_GENVAR 388
#define GEN_ITEM_CALC_SIGNAL_NAME 389
#define GEN_ITEM_CREATE_EXPR 390
#define GEN_ITEM_CREATE_SIG 391
#define GEN_ITEM_CREATE_STMT 392
#define GEN_ITEM_CREATE_INST 393
#define GEN_ITEM_CREATE_TFN 394
#define GEN_ITEM_CREATE_BIND 395
#define GEN_ITEM_RESIZE_STMTS_AND_SIGS 396
#define GEN_ITEM_ASSIGN_IDS 397
#define GEN_ITEM_DB_WRITE 398
#define GEN_ITEM_DB_WRITE_EXPR_TREE 399
#define GEN_ITEM_CONNECT 400
#define GEN_ITEM_RESOLVE 401
#define GEN_ITEM_BIND 402
#define GENERATE_RESOLVE 403
#define GENERATE_REMOVE_STMT_HELPER 404
#define GENERATE_REMOVE_STMT 405
#define GEN_ITEM_DEALLOC 406
#define INFO_INITIALIZE 407
#define INFO_SET_VECTOR_ELEM_SIZE 408
#define INFO_DB_WRITE 409
#define INFO_DB_READ 410
#define ARGS_DB_READ 411
#define MESSAGE_DB_READ 412
#define MERGED_CDD_DB_READ 413
#define INFO_DEALLOC 414
#define INSTANCE_DISPLAY_TREE_HELPER 415
#define INSTANCE_DISPLAY_TREE 416
#define INSTANCE_CREATE 417
#define INSTANCE_GEN_SCOPE 418
#define INSTANCE_COMPARE 419
#define INSTANCE_FIND_SCOPE 420
#define INSTANCE_FIND_BY_FUNIT 421
#define INSTANCE_FIND_SIGNAL_BY_EXCLUSION_ID 422
#define INSTANCE_FIND_EXPRESSION_BY_EXCLUSION_ID 423
#define INSTANCE_FIND_FSM_ARC_INDEX_BY_EXCLUSION_ID 424
#define INSTANCE_ADD_CHILD 425
#define INSTANCE_COPY 426
#define INSTANCE_ATTACH_CHILD 427
#define INSTANCE_PARSE_ADD 428
#define INSTANCE_RESOLVE_INST 429
#define INSTANCE_RESOLVE_HELPER 430
#define INSTANCE_RESOLVE 431
#define INSTANCE_READ_ADD 432
#define INSTANCE_DB_WRITE 433
#define INSTANCE_FLATTEN_HELPER 434
#define INSTANCE_FLATTEN 435
#define INSTANCE_REMOVE_STMT_BLKS_CALLING_STMT 436
#define INSTANCE_REMOVE_PARMS_WITH_EXPR 437
#define INSTANCE_DEALLOC_SINGLE 438
#define INSTANCE_DEALLOC_TREE 439
#define INSTANCE_DEALLOC 440
#define STMT_ITER_RESET 441
#define STMT_ITER_COPY 442
#define STMT_ITER_NEXT 443
#define STMT_ITER_REVERSE 444
#define STMT_ITER_FIND_HEAD 445
#define STMT_ITER_GET_NEXT_IN_ORDER 446
#define STMT_ITER_GET_LINE_BEFORE 447
#define LEXER_KEYWORD_1995_CODE 448
#define LEXER_KEYWORD_2001_CODE 449
#define LEXER_KEYWORD_SV_CODE 450
#define LEXER_CSTRING_A 451
#define LEXER_KEYWORD_A 452
#define LEXER_ESCAPED_NAME 453
#define LEXER_SYSTEM_CALL 454
#define LINE_DIRECTIVE 455
#define LINE_DIRECTIVE2 456
#define PROCESS_TIMESCALE_TOKEN 457
#define PROCESS_TIMESCALE 458
#define LEXER_YYWRAP 459
#define RESET_LEXER 460
#define CHECK_FOR_PRAGMA 461
#define LINE_GET_STATS 462
#define LINE_COLLECT 463
#define LINE_GET_FUNIT_SUMMARY 464
#define LINE_GET_INST_SUMMARY 465
#define LINE_DISPLAY_INSTANCE_SUMMARY 466
#define LINE_INSTANCE_SUMMARY 467
#define LINE_DISPLAY_FUNIT_SUMMARY 468
#define LINE_FUNIT_SUMMARY 469
#define LINE_DISPLAY_VERBOSE 470
#define LINE_INSTANCE_VERBOSE 471
#define LINE_FUNIT_VERBOSE 472
#define LINE_REPORT 473
#define STR_LINK_ADD 474
#define STMT_LINK_ADD_HEAD 475
#define STMT_LINK_ADD_TAIL 476
#define STMT_LINK_MERGE 477
#define EXP_LINK_ADD 478
#define SIG_LINK_ADD 479
#define FSM_LINK_ADD 480
#define FUNIT_LINK_ADD 481
#define GITEM_LINK_ADD 482
#define INST_LINK_ADD 483
#define STR_LINK_FIND 484
#define STMT_LINK_FIND 485
#define EXP_LINK_FIND 486
#define SIG_LINK_FIND 487
#define FSM_LINK_FIND 488
#define FUNIT_LINK_FIND 489
#define GITEM_LINK_FIND 490
#define INST_LINK_FIND_BY_SCOPE 491
#define INST_LINK_FIND_BY_FUNIT 492
#define STR_LINK_REMOVE 493
#define EXP_LINK_REMOVE 494
#define GITEM_LINK_REMOVE 495
#define FUNIT_LINK_REMOVE 496
#define INST_LINK_FLATTEN 497
#define STR_LINK_DELETE_LIST 498
#define STMT_LINK_UNLINK 499
#define STMT_LINK_DELETE_LIST 500
#define EXP_LINK_DELETE_LIST 501
#define SIG_LINK_DELETE_LIST 502
#define FSM_LINK_DELETE_LIST 503
#define FUNIT_LINK_DELETE_LIST 504
#define GITEM_LINK_DELETE_LIST 505
#define INST_LINK_DELETE_LIST 506
#define VCDID 507
#define VCD_CALLBACK 508
#define LXT_PARSE 509
#define LXT2_RD_EXPAND_INTEGER_TO_BITS 510
#define LXT2_RD_EXPAND_BITS_TO_INTEGER 511
#define LXT2_RD_ITER_RADIX 512
#define LXT2_RD_ITER_RADIX0 513
#define LXT2_RD_BUILD_RADIX 514
#define LXT2_RD_REGENERATE_PROCESS_MASK 515
#define LXT2_RD_PROCESS_BLOCK 516
#define LXT2_RD_INIT 517
#define LXT2_RD_CLOSE 518
#define LXT2_RD_GET_FACNAME 519
#define LXT2_RD_ITER_BLOCKS 520
#define LXT2_RD_LIMIT_TIME_RANGE 521
#define LXT2_RD_UNLIMIT_TIME_RANGE 522
#define MEMORY_GET_STAT 523
#define MEMORY_GET_STATS 524
#define MEMORY_GET_FUNIT_SUMMARY 525
#define MEMORY_GET_INST_SUMMARY 526
#define MEMORY_CREATE_PDIM_BIT_ARRAY 527
#define MEMORY_GET_MEM_COVERAGE 528
#define MEMORY_GET_COVERAGE 529
#define MEMORY_COLLECT 530
#define MEMORY_DISPLAY_TOGGLE_INSTANCE_SUMMARY 531
#define MEMORY_TOGGLE_INSTANCE_SUMMARY 532
#define MEMORY_DISPLAY_AE_INSTANCE_SUMMARY 533
#define MEMORY_AE_INSTANCE_SUMMARY 534
#define MEMORY_DISPLAY_TOGGLE_FUNIT_SUMMARY 535
#define MEMORY_TOGGLE_FUNIT_SUMMARY 536
#define MEMORY_DISPLAY_AE_FUNIT_SUMMARY 537
#define MEMORY_AE_FUNIT_SUMMARY 538
#define MEMORY_DISPLAY_MEMORY 539
#define MEMORY_DISPLAY_VERBOSE 540
#define MEMORY_INSTANCE_VERBOSE 541
#define MEMORY_FUNIT_VERBOSE 542
#define MEMORY_REPORT 543
#define COMMAND_MERGE 544
#define OBFUSCATE_SET_MODE 545
#define OBFUSCATE_NAME 546
#define OBFUSCATE_DEALLOC 547
#define OVL_IS_ASSERTION_NAME 548
#define OVL_IS_ASSERTION_MODULE 549
#define OVL_IS_COVERAGE_POINT 550
#define OVL_ADD_ASSERTIONS_TO_NO_SCORE_LIST 551
#define OVL_GET_FUNIT_STATS 552
#define OVL_GET_COVERAGE_POINT 553
#define OVL_DISPLAY_VERBOSE 554
#define OVL_COLLECT 555
#define OVL_GET_COVERAGE 556
#define MOD_PARM_FIND 557
#define MOD_PARM_FIND_EXPR_AND_REMOVE 558
#define MOD_PARM_ADD 559
#define INST_PARM_FIND 560
#define INST_PARM_ADD 561
#define INST_PARM_ADD_GENVAR 562
#define INST_PARM_BIND 563
#define DEFPARAM_ADD 564
#define DEFPARAM_DEALLOC 565
#define PARAM_FIND_AND_SET_EXPR_VALUE 566
#define PARAM_SET_SIG_SIZE 567
#define PARAM_SIZE_FUNCTION 568
#define PARAM_EXPR_EVAL 569
#define PARAM_HAS_OVERRIDE 570
#define PARAM_HAS_DEFPARAM 571
#define PARAM_RESOLVE_DECLARED 572
#define PARAM_RESOLVE_OVERRIDE 573
#define PARAM_RESOLVE 574
#define PARAM_DB_WRITE 575
#define MOD_PARM_DEALLOC 576
#define INST_PARM_DEALLOC 577
#define PARSE_READLINE 578
#define PARSE_DESIGN 579
#define PARSE_AND_SCORE_DUMPFILE 580
#define PARSER_PORT_DECLARATION_A 581
#define PARSER_PORT_DECLARATION_B 582
#define PARSER_PORT_DECLARATION_C 583
#define PARSER_STATIC_EXPR_PRIMARY_A 584
#define PARSER_STATIC_EXPR_PRIMARY_B 585
#define PARSER_EXPRESSION_LIST_A 586
#define PARSER_EXPRESSION_LIST_B 587
#define PARSER_EXPRESSION_LIST_C 588
#define PARSER_EXPRESSION_LIST_D 589
#define PARSER_IDENTIFIER_A 590
#define PARSER_GENERATE_CASE_ITEM_A 591
#define PARSER_GENERATE_CASE_ITEM_B 592
#define PARSER_GENERATE_CASE_ITEM_C 593
#define PARSER_STATEMENT_BEGIN_A 594
#define PARSER_STATEMENT_FORK_A 595
#define PARSER_STATEMENT_FOR_A 596
#define PARSER_CASE_ITEM_A 597
#define PARSER_CASE_ITEM_B 598
#define PARSER_CASE_ITEM_C 599
#define PARSER_DELAY_VALUE_A 600
#define PARSER_DELAY_VALUE_B 601
#define PARSER_PARAMETER_VALUE_BYNAME_A 602
#define PARSER_GATE_INSTANCE_A 603
#define PARSER_GATE_INSTANCE_B 604
#define PARSER_GATE_INSTANCE_C 605
#define PARSER_GATE_INSTANCE_D 606
#define PARSER_LIST_OF_NAMES_A 607
#define PARSER_LIST_OF_NAMES_B 608
#define VLERROR 609
#define VLWARN 610
#define PARSER_DEALLOC_SIG_RANGE 611
#define PARSER_COPY_CURR_RANGE 612
#define PARSER_COPY_RANGE_TO_CURR_RANGE 613
#define PARSER_EXPLICITLY_SET_CURR_RANGE 614
#define PARSER_IMPLICITLY_SET_CURR_RANGE 615
#define PARSER_CHECK_GENERATION 616
#define PERF_GEN_STATS 617
#define PERF_OUTPUT_MOD_STATS 618
#define PERF_OUTPUT_INST_REPORT_HELPER 619
#define PERF_OUTPUT_INST_REPORT 620
#define DEF_LOOKUP 621
#define IS_DEFINED 622
#define DEF_MATCH 623
#define DEF_START 624
#define DEFINE_MACRO 625
#define DO_DEFINE 626
#define DEF_IS_DONE 627
#define DEF_FINISH 628
#define DEF_UNDEFINE 629
#define INCLUDE_FILENAME 630
#define DO_INCLUDE 631
#define YYWRAP 632
#define RESET_PPLEXER 633
#define RACE_BLK_CREATE 634
#define RACE_FIND_HEAD_STATEMENT_CONTAINING_STATEMENT_HELPER 635
#define RACE_FIND_HEAD_STATEMENT_CONTAINING_STATEMENT 636
#define RACE_GET_HEAD_STATEMENT 637
#define RACE_FIND_HEAD_STATEMENT 638
#define RACE_CALC_STMT_BLK_TYPE 639
#define RACE_CALC_EXPR_ASSIGNMENT 640
#define RACE_CALC_ASSIGNMENTS 641
#define RACE_HANDLE_RACE_CONDITION 642
#define RACE_CHECK_ASSIGNMENT_TYPES 643
#define RACE_CHECK_ONE_BLOCK_ASSIGNMENT 644
#define RACE_CHECK_RACE_COUNT 645
#define RACE_CHECK_MODULES 646
#define RACE_DB_WRITE 647
#define RACE_DB_READ 648
#define RACE_GET_STATS 649
#define RACE_REPORT_SUMMARY 650
#define RACE_REPORT_VERBOSE 651
#define RACE_REPORT 652
#define RACE_COLLECT_LINES 653
#define RACE_BLK_DELETE_LIST 654
#define RANK_CREATE_COMP_CDD_COV 655
#define RANK_DEALLOC_COMP_CDD_COV 656
#define RANK_CHECK_INDEX 657
#define RANK_GATHER_SIGNAL_COV 658
#define RANK_GATHER_COMB_COV 659
#define RANK_GATHER_EXPRESSION_COV 660
#define RANK_GATHER_FSM_COV 661
#define RANK_CALC_NUM_CPS 662
#define RANK_GATHER_COMP_CDD_COV 663
#define RANK_READ_CDD 664
#define RANK_SELECTED_CDD_COV 665
#define RANK_PERFORM_WEIGHTED_SELECTION 666
#define RANK_PERFORM_GREEDY_SORT 667
#define RANK_COUNT_CPS 668
#define RANK_PERFORM 669
#define RANK_OUTPUT 670
#define COMMAND_RANK 671
#define REENTRANT_COUNT_AFU_BITS 672
#define REENTRANT_STORE_DATA_BITS 673
#define REENTRANT_RESTORE_DATA_BITS 674
#define REENTRANT_CREATE 675
#define REENTRANT_DEALLOC 676
#define REPORT_PARSE_METRICS 677
#define REPORT_PARSE_ARGS 678
#define REPORT_GATHER_INSTANCE_STATS 679
#define REPORT_GATHER_FUNIT_STATS 680
#define REPORT_PRINT_HEADER 681
#define REPORT_GENERATE 682
#define REPORT_READ_CDD_AND_READY 683
#define REPORT_CLOSE_CDD 684
#define REPORT_SAVE_CDD 685
#define REPORT_OUTPUT_EXCLUSION_REASON 686
#define COMMAND_REPORT 687
#define SCOPE_FIND_FUNIT_FROM_SCOPE 688
#define SCOPE_FIND_PARAM 689
#define SCOPE_FIND_SIGNAL 690
#define SCOPE_FIND_TASK_FUNCTION_NAMEDBLOCK 691
#define SCOPE_GET_PARENT_FUNIT 692
#define SCOPE_GET_PARENT_MODULE 693
#define SCORE_GENERATE_TOP_VPI_MODULE 694
#define SCORE_GENERATE_PLI_TAB_FILE 695
#define SCORE_PARSE_DEFINE 696
#define SCORE_ADD_ARG 697
#define SCORE_PARSE_ARGS 698
#define COMMAND_SCORE 699
#define SEARCH_INIT 700
#define SEARCH_ADD_INCLUDE_PATH 701
#define SEARCH_ADD_DIRECTORY_PATH 702
#define SEARCH_ADD_FILE 703
#define SEARCH_ADD_NO_SCORE_FUNIT 704
#define SEARCH_ADD_EXTENSIONS 705
#define SEARCH_FREE_LISTS 706
#define SIM_CURRENT_THREAD 707
#define SIM_THREAD_POP_HEAD 708
#define SIM_THREAD_INSERT_INTO_DELAY_QUEUE 709
#define SIM_THREAD_PUSH 710
#define SIM_EXPR_CHANGED 711
#define SIM_CREATE_THREAD 712
#define SIM_ADD_THREAD 713
#define SIM_KILL_THREAD 714
#define SIM_KILL_THREAD_WITH_FUNIT 715
#define SIM_ADD_STATICS 716
#define SIM_EXPRESSION 717
#define SIM_THREAD 718
#define SIM_SIMULATE 719
#define SIM_INITIALIZE 720
#define SIM_STOP 721
#define SIM_FINISH 722
#define SIM_DEALLOC 723
#define STATISTIC_CREATE 724
#define STATISTIC_IS_EMPTY 725
#define STATISTIC_DEALLOC 726
#define STATEMENT_CREATE 727
#define STATEMENT_QUEUE_ADD 728
#define STATEMENT_QUEUE_COMPARE 729
#define STATEMENT_SIZE_ELEMENTS 730
#define STATEMENT_DB_WRITE 731
#define STATEMENT_DB_WRITE_TREE 732
#define STATEMENT_DB_WRITE_EXPR_TREE 733
#define STATEMENT_DB_READ 734
#define STATEMENT_ASSIGN_EXPR_IDS 735
#define STATEMENT_CONNECT 736
#define STATEMENT_GET_LAST_LINE_HELPER 737
#define STATEMENT_GET_LAST_LINE 738
#define STATEMENT_FIND_RHS_SIGS 739
#define STATEMENT_FIND_HEAD_STATEMENT 740
#define STATEMENT_FIND_STATEMENT 741
#define STATEMENT_CONTAINS_EXPR_CALLING_STMT 742
#define STATEMENT_DEALLOC_RECURSIVE 743
#define STATEMENT_DEALLOC 744
#define STATIC_EXPR_GEN_UNARY 745
#define STATIC_EXPR_GEN 746
#define STATIC_EXPR_CALC_LSB_AND_WIDTH_PRE 747
#define STATIC_EXPR_CALC_LSB_AND_WIDTH_POST 748
#define STATIC_EXPR_DEALLOC 749
#define STRUCT_UNION_LENGTH 750
#define STRUCT_UNION_ADD_MEMBER 751
#define STRUCT_UNION_ADD_MEMBER_VOID 752
#define STRUCT_UNION_ADD_MEMBER_SIG 753
#define STRUCT_UNION_ADD_MEMBER_TYPEDEF 754
#define STRUCT_UNION_ADD_MEMBER_ENUM 755
#define STRUCT_UNION_ADD_MEMBER_STRUCT_UNION 756
#define STRUCT_UNION_CREATE 757
#define STRUCT_UNION_MEMBER_DEALLOC 758
#define STRUCT_UNION_DEALLOC 759
#define STRUCT_UNION_DEALLOC_LIST 760
#define SYMTABLE_ADD_SYM_SIG 761
#define SYMTABLE_INIT 762
#define SYMTABLE_CREATE 763
#define SYMTABLE_ADD 764
#define SYMTABLE_SET_VALUE 765
#define SYMTABLE_ASSIGN 766
#define SYMTABLE_DEALLOC 767
#define TCL_FUNC_GET_RACE_REASON_MSGS 768
#define TCL_FUNC_GET_FUNIT_LIST 769
#define TCL_FUNC_GET_INSTANCES 770
#define TCL_FUNC_GET_INSTANCE_LIST 771
#define TCL_FUNC_IS_FUNIT 772
#define TCL_FUNC_GET_FUNIT 773
#define TCL_FUNC_GET_INST 774
#define TCL_FUNC_GET_FUNIT_NAME 775
#define TCL_FUNC_GET_FILENAME 776
#define TCL_FUNC_INST_SCOPE 777
#define TCL_FUNC_GET_FUNIT_START_AND_END 778
#define TCL_FUNC_COLLECT_UNCOVERED_LINES 779
#define TCL_FUNC_COLLECT_COVERED_LINES 780
#define TCL_FUNC_COLLECT_RACE_LINES 781
#define TCL_FUNC_COLLECT_UNCOVERED_TOGGLES 782
#define TCL_FUNC_COLLECT_COVERED_TOGGLES 783
#define TCL_FUNC_COLLECT_UNCOVERED_MEMORIES 784
#define TCL_FUNC_COLLECT_COVERED_MEMORIES 785
#define TCL_FUNC_GET_TOGGLE_COVERAGE 786
#define TCL_FUNC_GET_MEMORY_COVERAGE 787
#define TCL_FUNC_COLLECT_UNCOVERED_COMBS 788
#define TCL_FUNC_COLLECT_COVERED_COMBS 789
#define TCL_FUNC_GET_COMB_EXPRESSION 790
#define TCL_FUNC_GET_COMB_COVERAGE 791
#define TCL_FUNC_COLLECT_UNCOVERED_FSMS 792
#define TCL_FUNC_COLLECT_COVERED_FSMS 793
#define TCL_FUNC_GET_FSM_COVERAGE 794
#define TCL_FUNC_COLLECT_UNCOVERED_ASSERTIONS 795
#define TCL_FUNC_COLLECT_COVERED_ASSERTIONS 796
#define TCL_FUNC_GET_ASSERT_COVERAGE 797
#define TCL_FUNC_OPEN_CDD 798
#define TCL_FUNC_CLOSE_CDD 799
#define TCL_FUNC_SAVE_CDD 800
#define TCL_FUNC_MERGE_CDD 801
#define TCL_FUNC_GET_LINE_SUMMARY 802
#define TCL_FUNC_GET_TOGGLE_SUMMARY 803
#define TCL_FUNC_GET_MEMORY_SUMMARY 804
#define TCL_FUNC_GET_COMB_SUMMARY 805
#define TCL_FUNC_GET_FSM_SUMMARY 806
#define TCL_FUNC_GET_ASSERT_SUMMARY 807
#define TCL_FUNC_PREPROCESS_VERILOG 808
#define TCL_FUNC_GET_SCORE_PATH 809
#define TCL_FUNC_GET_INCLUDE_PATHNAME 810
#define TCL_FUNC_GET_GENERATION 811
#define TCL_FUNC_SET_LINE_EXCLUDE 812
#define TCL_FUNC_SET_TOGGLE_EXCLUDE 813
#define TCL_FUNC_SET_MEMORY_EXCLUDE 814
#define TCL_FUNC_SET_COMB_EXCLUDE 815
#define TCL_FUNC_FSM_EXCLUDE 816
#define TCL_FUNC_SET_ASSERT_EXCLUDE 817
#define TCL_FUNC_GENERATE_REPORT 818
#define TCL_FUNC_INITIALIZE 819
#define TOGGLE_GET_STATS 820
#define TOGGLE_COLLECT 821
#define TOGGLE_GET_COVERAGE 822
#define TOGGLE_GET_FUNIT_SUMMARY 823
#define TOGGLE_GET_INST_SUMMARY 824
#define TOGGLE_DISPLAY_INSTANCE_SUMMARY 825
#define TOGGLE_INSTANCE_SUMMARY 826
#define TOGGLE_DISPLAY_FUNIT_SUMMARY 827
#define TOGGLE_FUNIT_SUMMARY 828
#define TOGGLE_DISPLAY_VERBOSE 829
#define TOGGLE_INSTANCE_VERBOSE 830
#define TOGGLE_FUNIT_VERBOSE 831
#define TOGGLE_REPORT 832
#define TREE_ADD 833
#define TREE_FIND 834
#define TREE_REMOVE 835
#define TREE_DEALLOC 836
#define CHECK_OPTION_VALUE 837
#define IS_VARIABLE 838
#define IS_FUNC_UNIT 839
#define IS_LEGAL_FILENAME 840
#define GET_BASENAME 841
#define GET_DIRNAME 842
#define DIRECTORY_EXISTS 843
#define DIRECTORY_LOAD 844
#define FILE_EXISTS 845
#define UTIL_READLINE 846
#define GET_QUOTED_STRING 847
#define SUBSTITUTE_ENV_VARS 848
#define SCOPE_EXTRACT_FRONT 849
#define SCOPE_EXTRACT_BACK 850
#define SCOPE_EXTRACT_SCOPE 851
#define SCOPE_GEN_PRINTABLE 852
#define SCOPE_COMPARE 853
#define SCOPE_LOCAL 854
#define CONVERT_FILE_TO_MODULE 855
#define GET_NEXT_VFILE 856
#define GEN_SPACE 857
#define GET_FUNIT_TYPE 858
#define CALC_MISS_PERCENT 859
#define READ_COMMAND_FILE 860
#define VCD_PARSE_DEF_IGNORE 861
#define VCD_PARSE_DEF_VAR 862
#define VCD_PARSE_DEF_SCOPE 863
#define VCD_PARSE_DEF 864
#define VCD_PARSE_SIM_VECTOR 865
#define VCD_PARSE_SIM_IGNORE 866
#define VCD_PARSE_SIM 867
#define VCD_PARSE 868
#define VECTOR_INIT 869
#define VECTOR_CREATE 870
#define VECTOR_COPY 871
#define VECTOR_COPY_RANGE 872
#define VECTOR_CLONE 873
#define VECTOR_DB_WRITE 874
#define VECTOR_DB_READ 875
#define VECTOR_DB_MERGE 876
#define VECTOR_MERGE 877
#define VECTOR_GET_EVAL_A 878
#define VECTOR_GET_EVAL_B 879
#define VECTOR_GET_EVAL_C 880
#define VECTOR_GET_EVAL_D 881
#define VECTOR_GET_EVAL_AB_COUNT 882
#define VECTOR_GET_EVAL_ABC_COUNT 883
#define VECTOR_GET_EVAL_ABCD_COUNT 884
#define VECTOR_GET_TOGGLE01_ULONG 885
#define VECTOR_GET_TOGGLE10_ULONG 886
#define VECTOR_DISPLAY_TOGGLE01_ULONG 887
#define VECTOR_DISPLAY_TOGGLE10_ULONG 888
#define VECTOR_TOGGLE_COUNT 889
#define VECTOR_MEM_RW_COUNT 890
#define VECTOR_SET_ASSIGNED 891
#define VECTOR_SET_COVERAGE_AND_ASSIGN 892
#define VECTOR_GET_SIGN_EXTEND_VECTOR_ULONG 893
#define VECTOR_SIGN_EXTEND_ULONG 894
#define VECTOR_LSHIFT_ULONG 895
#define VECTOR_RSHIFT_ULONG 896
#define VECTOR_SET_VALUE 897
#define VECTOR_PART_SELECT_PULL 898
#define VECTOR_PART_SELECT_PUSH 899
#define VECTOR_SET_UNARY_EVALS 900
#define VECTOR_SET_AND_COMB_EVALS 901
#define VECTOR_SET_OR_COMB_EVALS 902
#define VECTOR_SET_OTHER_COMB_EVALS 903
#define VECTOR_IS_UKNOWN 904
#define VECTOR_IS_NOT_ZERO 905
#define VECTOR_SET_TO_X 906
#define VECTOR_TO_INT 907
#define VECTOR_TO_UINT64 908
#define VECTOR_TO_SIM_TIME 909
#define VECTOR_FROM_INT 910
#define VECTOR_FROM_UINT64 911
#define VECTOR_SET_STATIC 912
#define VECTOR_TO_STRING 913
#define VECTOR_FROM_STRING 914
#define VECTOR_VCD_ASSIGN 915
#define VECTOR_BITWISE_AND_OP 916
#define VECTOR_BITWISE_NAND_OP 917
#define VECTOR_BITWISE_OR_OP 918
#define VECTOR_BITWISE_NOR_OP 919
#define VECTOR_BITWISE_XOR_OP 920
#define VECTOR_BITWISE_NXOR_OP 921
#define VECTOR_OP_LT 922
#define VECTOR_OP_LE 923
#define VECTOR_OP_GT 924
#define VECTOR_OP_GE 925
#define VECTOR_OP_EQ 926
#define VECTOR_CEQ_ULONG 927
#define VECTOR_OP_CEQ 928
#define VECTOR_OP_CXEQ 929
#define VECTOR_OP_CZEQ 930
#define VECTOR_OP_NE 931
#define VECTOR_OP_CNE 932
#define VECTOR_OP_LOR 933
#define VECTOR_OP_LAND 934
#define VECTOR_OP_LSHIFT 935
#define VECTOR_OP_RSHIFT 936
#define VECTOR_OP_ARSHIFT 937
#define VECTOR_OP_ADD 938
#define VECTOR_OP_NEGATE 939
#define VECTOR_OP_SUBTRACT 940
#define VECTOR_OP_MULTIPLY 941
#define VECTOR_OP_DIVIDE 942
#define VECTOR_OP_MODULUS 943
#define VECTOR_OP_INC 944
#define VECTOR_OP_DEC 945
#define VECTOR_UNARY_INV 946
#define VECTOR_UNARY_AND 947
#define VECTOR_UNARY_NAND 948
#define VECTOR_UNARY_OR 949
#define VECTOR_UNARY_NOR 950
#define VECTOR_UNARY_XOR 951
#define VECTOR_UNARY_NXOR 952
#define VECTOR_UNARY_NOT 953
#define VECTOR_OP_EXPAND 954
#define VECTOR_OP_LIST 955
#define VECTOR_DEALLOC_VALUE 956
#define VECTOR_DEALLOC 957
#define SYM_VALUE_STORE 958
#define ADD_SYM_VALUES_TO_SIM 959
#define COVERED_VALUE_CHANGE 960
#define COVERED_END_OF_SIM 961
#define COVERED_CB_ERROR_HANDLER 962
#define GEN_NEXT_SYMBOL 963
#define COVERED_CREATE_VALUE_CHANGE_CB 964
#define COVERED_PARSE_TASK_FUNC 965
#define COVERED_PARSE_SIGNALS 966
#define COVERED_PARSE_INSTANCE 967
#define COVERED_SIM_CALLTF 968
#define COVERED_REGISTER 969
#define VSIGNAL_INIT 970
#define VSIGNAL_CREATE 971
#define VSIGNAL_CREATE_VEC 972
#define VSIGNAL_DUPLICATE 973
#define VSIGNAL_DB_WRITE 974
#define VSIGNAL_DB_READ 975
#define VSIGNAL_DB_MERGE 976
#define VSIGNAL_MERGE 977
#define VSIGNAL_PROPAGATE 978
#define VSIGNAL_VCD_ASSIGN 979
#define VSIGNAL_ADD_EXPRESSION 980
#define VSIGNAL_FROM_STRING 981
#define VSIGNAL_CALC_WIDTH_FOR_EXPR 982
#define VSIGNAL_CALC_LSB_FOR_EXPR 983
#define VSIGNAL_DEALLOC 984

extern profiler profiles[NUM_PROFILES];
#endif

extern unsigned int profile_index;

#endif

