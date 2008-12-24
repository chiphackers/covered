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

#define NUM_PROFILES 1114

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
#define CODEGEN_GEN_EXPR_ONE_LINE 49
#define COMBINATION_CALC_DEPTH 50
#define COMBINATION_DOES_MULTI_EXP_NEED_UL 51
#define COMBINATION_MULTI_EXPR_CALC 52
#define COMBINATION_IS_EXPR_MULTI_NODE 53
#define COMBINATION_GET_TREE_STATS 54
#define COMBINATION_RESET_COUNTED_EXPRS 55
#define COMBINATION_RESET_COUNTED_EXPR_TREE 56
#define COMBINATION_GET_STATS 57
#define COMBINATION_GET_FUNIT_SUMMARY 58
#define COMBINATION_GET_INST_SUMMARY 59
#define COMBINATION_DISPLAY_INSTANCE_SUMMARY 60
#define COMBINATION_INSTANCE_SUMMARY 61
#define COMBINATION_DISPLAY_FUNIT_SUMMARY 62
#define COMBINATION_FUNIT_SUMMARY 63
#define COMBINATION_DRAW_LINE 64
#define COMBINATION_DRAW_CENTERED_LINE 65
#define COMBINATION_UNDERLINE_TREE 66
#define COMBINATION_PREP_LINE 67
#define COMBINATION_UNDERLINE 68
#define COMBINATION_UNARY 69
#define COMBINATION_EVENT 70
#define COMBINATION_TWO_VARS 71
#define COMBINATION_MULTI_VAR_EXPRS 72
#define COMBINATION_MULTI_EXPR_OUTPUT_LENGTH 73
#define COMBINATION_MULTI_EXPR_OUTPUT 74
#define COMBINATION_MULTI_VARS 75
#define COMBINATION_GET_MISSED_EXPR 76
#define COMBINATION_LIST_MISSED 77
#define COMBINATION_OUTPUT_EXPR 78
#define COMBINATION_DISPLAY_VERBOSE 79
#define COMBINATION_INSTANCE_VERBOSE 80
#define COMBINATION_FUNIT_VERBOSE 81
#define COMBINATION_COLLECT 82
#define COMBINATION_GET_EXCLUDE_LIST 83
#define COMBINATION_GET_EXPRESSION 84
#define COMBINATION_GET_COVERAGE 85
#define COMBINATION_REPORT 86
#define DB_CREATE 87
#define DB_CLOSE 88
#define DB_CHECK_FOR_TOP_MODULE 89
#define DB_WRITE 90
#define DB_READ 91
#define DB_MERGE_INSTANCE_TREES 92
#define DB_MERGE_FUNITS 93
#define DB_SCALE_TO_PRECISION 94
#define DB_CREATE_UNNAMED_SCOPE 95
#define DB_IS_UNNAMED_SCOPE 96
#define DB_SET_TIMESCALE 97
#define DB_FIND_AND_SET_CURR_FUNIT 98
#define DB_GET_CURR_FUNIT 99
#define DB_GET_EXCLUSION_ID_SIZE 100
#define DB_GEN_EXCLUSION_ID 101
#define DB_ADD_FILE_VERSION 102
#define DB_OUTPUT_DUMPVARS 103
#define DB_ADD_INSTANCE 104
#define DB_ADD_MODULE 105
#define DB_END_MODULE 106
#define DB_ADD_FUNCTION_TASK_NAMEDBLOCK 107
#define DB_END_FUNCTION_TASK_NAMEDBLOCK 108
#define DB_ADD_DECLARED_PARAM 109
#define DB_ADD_OVERRIDE_PARAM 110
#define DB_ADD_VECTOR_PARAM 111
#define DB_ADD_DEFPARAM 112
#define DB_ADD_SIGNAL 113
#define DB_ADD_ENUM 114
#define DB_END_ENUM_LIST 115
#define DB_ADD_TYPEDEF 116
#define DB_FIND_SIGNAL 117
#define DB_ADD_GEN_ITEM_BLOCK 118
#define DB_FIND_GEN_ITEM 119
#define DB_FIND_TYPEDEF 120
#define DB_GET_CURR_GEN_BLOCK 121
#define DB_CURR_SIGNAL_COUNT 122
#define DB_CREATE_EXPRESSION 123
#define DB_BIND_EXPR_TREE 124
#define DB_CREATE_EXPR_FROM_STATIC 125
#define DB_ADD_EXPRESSION 126
#define DB_CREATE_SENSITIVITY_LIST 127
#define DB_PARALLELIZE_STATEMENT 128
#define DB_CREATE_STATEMENT 129
#define DB_ADD_STATEMENT 130
#define DB_REMOVE_STATEMENT_FROM_CURRENT_FUNIT 131
#define DB_REMOVE_STATEMENT 132
#define DB_CONNECT_STATEMENT_TRUE 133
#define DB_CONNECT_STATEMENT_FALSE 134
#define DB_GEN_ITEM_CONNECT_TRUE 135
#define DB_GEN_ITEM_CONNECT_FALSE 136
#define DB_GEN_ITEM_CONNECT 137
#define DB_STATEMENT_CONNECT 138
#define DB_CREATE_ATTR_PARAM 139
#define DB_PARSE_ATTRIBUTE 140
#define DB_REMOVE_STMT_BLKS_CALLING_STATEMENT 141
#define DB_GEN_CURR_INST_SCOPE 142
#define DB_SYNC_CURR_INSTANCE 143
#define DB_SET_VCD_SCOPE 144
#define DB_VCD_UPSCOPE 145
#define DB_ASSIGN_SYMBOL 146
#define DB_SET_SYMBOL_CHAR 147
#define DB_SET_SYMBOL_STRING 148
#define DB_DO_TIMESTEP 149
#define ENUMERATE_ADD_ITEM 150
#define ENUMERATE_END_LIST 151
#define ENUMERATE_RESOLVE 152
#define ENUMERATE_DEALLOC 153
#define ENUMERATE_DEALLOC_LIST 154
#define EXCLUDE_EXPR_ASSIGN_AND_RECALC 155
#define EXCLUDE_SIG_ASSIGN_AND_RECALC 156
#define EXCLUDE_ARC_ASSIGN_AND_RECALC 157
#define EXCLUDE_ADD_EXCLUDE_REASON 158
#define EXCLUDE_REMOVE_EXCLUDE_REASON 159
#define EXCLUDE_IS_LINE_EXCLUDED 160
#define EXCLUDE_SET_LINE_EXCLUDE 161
#define EXCLUDE_IS_TOGGLE_EXCLUDED 162
#define EXCLUDE_SET_TOGGLE_EXCLUDE 163
#define EXCLUDE_IS_COMB_EXCLUDED 164
#define EXCLUDE_SET_COMB_EXCLUDE 165
#define EXCLUDE_IS_FSM_EXCLUDED 166
#define EXCLUDE_SET_FSM_EXCLUDE 167
#define EXCLUDE_IS_ASSERT_EXCLUDED 168
#define EXCLUDE_SET_ASSERT_EXCLUDE 169
#define EXCLUDE_FIND_EXCLUDE_REASON 170
#define EXCLUDE_DB_WRITE 171
#define EXCLUDE_DB_READ 172
#define EXCLUDE_RESOLVE_REASON 173
#define EXCLUDE_DB_MERGE 174
#define EXCLUDE_MERGE 175
#define EXCLUDE_FIND_SIGNAL 176
#define EXCLUDE_FIND_EXPRESSION 177
#define EXCLUDE_FIND_FSM_ARC 178
#define EXCLUDE_FORMAT_REASON 179
#define EXCLUDED_GET_MESSAGE 180
#define EXCLUDE_HANDLE_EXCLUDE_REASON 181
#define EXCLUDE_PRINT_EXCLUSION 182
#define EXCLUDE_LINE_FROM_ID 183
#define EXCLUDE_TOGGLE_FROM_ID 184
#define EXCLUDE_MEMORY_FROM_ID 185
#define EXCLUDE_EXPR_FROM_ID 186
#define EXCLUDE_FSM_FROM_ID 187
#define EXCLUDE_ASSERT_FROM_ID 188
#define EXCLUDE_APPLY_EXCLUSIONS 189
#define COMMAND_EXCLUDE 190
#define EXPRESSION_CREATE_TMP_VECS 191
#define EXPRESSION_CREATE_VALUE 192
#define EXPRESSION_CREATE 193
#define EXPRESSION_SET_VALUE 194
#define EXPRESSION_SET_SIGNED 195
#define EXPRESSION_RESIZE 196
#define EXPRESSION_GET_ID 197
#define EXPRESSION_GET_FIRST_LINE_EXPR 198
#define EXPRESSION_GET_LAST_LINE_EXPR 199
#define EXPRESSION_GET_CURR_DIMENSION 200
#define EXPRESSION_FIND_RHS_SIGS 201
#define EXPRESSION_FIND_PARAMS 202
#define EXPRESSION_FIND_ULINE_ID 203
#define EXPRESSION_FIND_EXPR 204
#define EXPRESSION_CONTAINS_EXPR_CALLING_STMT 205
#define EXPRESSION_GET_ROOT_STATEMENT 206
#define EXPRESSION_ASSIGN_EXPR_IDS 207
#define EXPRESSION_DB_WRITE 208
#define EXPRESSION_DB_WRITE_TREE 209
#define EXPRESSION_DB_READ 210
#define EXPRESSION_DB_MERGE 211
#define EXPRESSION_MERGE 212
#define EXPRESSION_STRING_OP 213
#define EXPRESSION_STRING 214
#define EXPRESSION_OP_FUNC__XOR 215
#define EXPRESSION_OP_FUNC__XOR_A 216
#define EXPRESSION_OP_FUNC__MULTIPLY 217
#define EXPRESSION_OP_FUNC__MULTIPLY_A 218
#define EXPRESSION_OP_FUNC__DIVIDE 219
#define EXPRESSION_OP_FUNC__DIVIDE_A 220
#define EXPRESSION_OP_FUNC__MOD 221
#define EXPRESSION_OP_FUNC__MOD_A 222
#define EXPRESSION_OP_FUNC__ADD 223
#define EXPRESSION_OP_FUNC__ADD_A 224
#define EXPRESSION_OP_FUNC__SUBTRACT 225
#define EXPRESSION_OP_FUNC__SUB_A 226
#define EXPRESSION_OP_FUNC__AND 227
#define EXPRESSION_OP_FUNC__AND_A 228
#define EXPRESSION_OP_FUNC__OR 229
#define EXPRESSION_OP_FUNC__OR_A 230
#define EXPRESSION_OP_FUNC__NAND 231
#define EXPRESSION_OP_FUNC__NOR 232
#define EXPRESSION_OP_FUNC__NXOR 233
#define EXPRESSION_OP_FUNC__LT 234
#define EXPRESSION_OP_FUNC__GT 235
#define EXPRESSION_OP_FUNC__LSHIFT 236
#define EXPRESSION_OP_FUNC__LSHIFT_A 237
#define EXPRESSION_OP_FUNC__RSHIFT 238
#define EXPRESSION_OP_FUNC__RSHIFT_A 239
#define EXPRESSION_OP_FUNC__ARSHIFT 240
#define EXPRESSION_OP_FUNC__ARSHIFT_A 241
#define EXPRESSION_OP_FUNC__TIME 242
#define EXPRESSION_OP_FUNC__RANDOM 243
#define EXPRESSION_OP_FUNC__SASSIGN 244
#define EXPRESSION_OP_FUNC__SRANDOM 245
#define EXPRESSION_OP_FUNC__URANDOM 246
#define EXPRESSION_OP_FUNC__URANDOM_RANGE 247
#define EXPRESSION_OP_FUNC__REALTOBITS 248
#define EXPRESSION_OP_FUNC__BITSTOREAL 249
#define EXPRESSION_OP_FUNC__SHORTREALTOBITS 250
#define EXPRESSION_OP_FUNC__BITSTOSHORTREAL 251
#define EXPRESSION_OP_FUNC__ITOR 252
#define EXPRESSION_OP_FUNC__RTOI 253
#define EXPRESSION_OP_FUNC__TEST_PLUSARGS 254
#define EXPRESSION_OP_FUNC__VALUE_PLUSARGS 255
#define EXPRESSION_OP_FUNC__EQ 256
#define EXPRESSION_OP_FUNC__CEQ 257
#define EXPRESSION_OP_FUNC__LE 258
#define EXPRESSION_OP_FUNC__GE 259
#define EXPRESSION_OP_FUNC__NE 260
#define EXPRESSION_OP_FUNC__CNE 261
#define EXPRESSION_OP_FUNC__LOR 262
#define EXPRESSION_OP_FUNC__LAND 263
#define EXPRESSION_OP_FUNC__COND 264
#define EXPRESSION_OP_FUNC__COND_SEL 265
#define EXPRESSION_OP_FUNC__UINV 266
#define EXPRESSION_OP_FUNC__UAND 267
#define EXPRESSION_OP_FUNC__UNOT 268
#define EXPRESSION_OP_FUNC__UOR 269
#define EXPRESSION_OP_FUNC__UXOR 270
#define EXPRESSION_OP_FUNC__UNAND 271
#define EXPRESSION_OP_FUNC__UNOR 272
#define EXPRESSION_OP_FUNC__UNXOR 273
#define EXPRESSION_OP_FUNC__NULL 274
#define EXPRESSION_OP_FUNC__SIG 275
#define EXPRESSION_OP_FUNC__SBIT 276
#define EXPRESSION_OP_FUNC__MBIT 277
#define EXPRESSION_OP_FUNC__EXPAND 278
#define EXPRESSION_OP_FUNC__LIST 279
#define EXPRESSION_OP_FUNC__CONCAT 280
#define EXPRESSION_OP_FUNC__PEDGE 281
#define EXPRESSION_OP_FUNC__NEDGE 282
#define EXPRESSION_OP_FUNC__AEDGE 283
#define EXPRESSION_OP_FUNC__EOR 284
#define EXPRESSION_OP_FUNC__SLIST 285
#define EXPRESSION_OP_FUNC__DELAY 286
#define EXPRESSION_OP_FUNC__TRIGGER 287
#define EXPRESSION_OP_FUNC__CASE 288
#define EXPRESSION_OP_FUNC__CASEX 289
#define EXPRESSION_OP_FUNC__CASEZ 290
#define EXPRESSION_OP_FUNC__DEFAULT 291
#define EXPRESSION_OP_FUNC__BASSIGN 292
#define EXPRESSION_OP_FUNC__FUNC_CALL 293
#define EXPRESSION_OP_FUNC__TASK_CALL 294
#define EXPRESSION_OP_FUNC__NB_CALL 295
#define EXPRESSION_OP_FUNC__FORK 296
#define EXPRESSION_OP_FUNC__JOIN 297
#define EXPRESSION_OP_FUNC__DISABLE 298
#define EXPRESSION_OP_FUNC__REPEAT 299
#define EXPRESSION_OP_FUNC__EXPONENT 300
#define EXPRESSION_OP_FUNC__PASSIGN 301
#define EXPRESSION_OP_FUNC__MBIT_POS 302
#define EXPRESSION_OP_FUNC__MBIT_NEG 303
#define EXPRESSION_OP_FUNC__NEGATE 304
#define EXPRESSION_OP_FUNC__IINC 305
#define EXPRESSION_OP_FUNC__PINC 306
#define EXPRESSION_OP_FUNC__IDEC 307
#define EXPRESSION_OP_FUNC__PDEC 308
#define EXPRESSION_OP_FUNC__DLY_ASSIGN 309
#define EXPRESSION_OP_FUNC__DLY_OP 310
#define EXPRESSION_OP_FUNC__REPEAT_DLY 311
#define EXPRESSION_OP_FUNC__DIM 312
#define EXPRESSION_OP_FUNC__WAIT 313
#define EXPRESSION_OP_FUNC__FINISH 314
#define EXPRESSION_OP_FUNC__STOP 315
#define EXPRESSION_OPERATE 316
#define EXPRESSION_OPERATE_RECURSIVELY 317
#define EXPRESSION_VCD_ASSIGN 318
#define EXPRESSION_IS_STATIC_ONLY_HELPER 319
#define EXPRESSION_IS_ASSIGNED 320
#define EXPRESSION_IS_BIT_SELECT 321
#define EXPRESSION_IS_LAST_SELECT 322
#define EXPRESSION_IS_IN_RASSIGN 323
#define EXPRESSION_SET_ASSIGNED 324
#define EXPRESSION_SET_CHANGED 325
#define EXPRESSION_ASSIGN 326
#define EXPRESSION_DEALLOC 327
#define FSM_CREATE 328
#define FSM_ADD_ARC 329
#define FSM_CREATE_TABLES 330
#define FSM_DB_WRITE 331
#define FSM_DB_READ 332
#define FSM_DB_MERGE 333
#define FSM_MERGE 334
#define FSM_TABLE_SET 335
#define FSM_VCD_ASSIGN 336
#define FSM_GET_STATS 337
#define FSM_GET_FUNIT_SUMMARY 338
#define FSM_GET_INST_SUMMARY 339
#define FSM_GATHER_SIGNALS 340
#define FSM_COLLECT 341
#define FSM_GET_COVERAGE 342
#define FSM_DISPLAY_INSTANCE_SUMMARY 343
#define FSM_INSTANCE_SUMMARY 344
#define FSM_DISPLAY_FUNIT_SUMMARY 345
#define FSM_FUNIT_SUMMARY 346
#define FSM_DISPLAY_STATE_VERBOSE 347
#define FSM_DISPLAY_ARC_VERBOSE 348
#define FSM_DISPLAY_VERBOSE 349
#define FSM_INSTANCE_VERBOSE 350
#define FSM_FUNIT_VERBOSE 351
#define FSM_REPORT 352
#define FSM_DEALLOC 353
#define FSM_ARG_PARSE_STATE 354
#define FSM_ARG_PARSE 355
#define FSM_ARG_PARSE_VALUE 356
#define FSM_ARG_PARSE_TRANS 357
#define FSM_ARG_PARSE_ATTR 358
#define FSM_VAR_ADD 359
#define FSM_VAR_IS_OUTPUT_STATE 360
#define FSM_VAR_BIND_EXPR 361
#define FSM_VAR_ADD_EXPR 362
#define FSM_VAR_BIND_STMT 363
#define FSM_VAR_BIND_ADD 364
#define FSM_VAR_STMT_ADD 365
#define FSM_VAR_BIND 366
#define FSM_VAR_DEALLOC 367
#define FSM_VAR_REMOVE 368
#define FSM_VAR_CLEANUP 369
#define FUNC_ITER_DISPLAY 370
#define FUNC_ITER_SORT 371
#define FUNC_ITER_COUNT_STMT_ITERS 372
#define FUNC_ITER_ADD_STMT_ITERS 373
#define FUNC_ITER_ADD_SIG_LINKS 374
#define FUNC_ITER_INIT 375
#define FUNC_ITER_RESET 376
#define FUNC_ITER_GET_NEXT_STATEMENT 377
#define FUNC_ITER_GET_NEXT_SIGNAL 378
#define FUNC_ITER_DEALLOC 379
#define FUNIT_INIT 380
#define FUNIT_CREATE 381
#define FUNIT_GET_CURR_MODULE 382
#define FUNIT_GET_CURR_MODULE_SAFE 383
#define FUNIT_GET_CURR_FUNCTION 384
#define FUNIT_GET_CURR_TASK 385
#define FUNIT_GET_PORT_COUNT 386
#define FUNIT_FIND_PARAM 387
#define FUNIT_FIND_SIGNAL 388
#define FUNIT_REMOVE_STMT_BLKS_CALLING_STMT 389
#define FUNIT_GEN_TASK_FUNCTION_NAMEDBLOCK_NAME 390
#define FUNIT_SIZE_ELEMENTS 391
#define FUNIT_DB_WRITE 392
#define FUNIT_DB_READ 393
#define FUNIT_VERSION_DB_READ 394
#define FUNIT_DB_MERGE 395
#define FUNIT_MERGE 396
#define FUNIT_FLATTEN_NAME 397
#define FUNIT_FIND_BY_ID 398
#define FUNIT_IS_TOP_MODULE 399
#define FUNIT_IS_UNNAMED 400
#define FUNIT_IS_UNNAMED_CHILD_OF 401
#define FUNIT_IS_CHILD_OF 402
#define FUNIT_DISPLAY_SIGNALS 403
#define FUNIT_DISPLAY_EXPRESSIONS 404
#define STATEMENT_ADD_THREAD 405
#define FUNIT_PUSH_THREADS 406
#define STATEMENT_DELETE_THREAD 407
#define FUNIT_OUTPUT_DUMPVARS 408
#define FUNIT_CLEAN 409
#define FUNIT_DEALLOC 410
#define GEN_ITEM_STRINGIFY 411
#define GEN_ITEM_DISPLAY 412
#define GEN_ITEM_DISPLAY_BLOCK_HELPER 413
#define GEN_ITEM_DISPLAY_BLOCK 414
#define GEN_ITEM_COMPARE 415
#define GEN_ITEM_FIND 416
#define GEN_ITEM_REMOVE_IF_CONTAINS_EXPR_CALLING_STMT 417
#define GEN_ITEM_GET_GENVAR 418
#define GEN_ITEM_VARNAME_CONTAINS_GENVAR 419
#define GEN_ITEM_CALC_SIGNAL_NAME 420
#define GEN_ITEM_CREATE_EXPR 421
#define GEN_ITEM_CREATE_SIG 422
#define GEN_ITEM_CREATE_STMT 423
#define GEN_ITEM_CREATE_INST 424
#define GEN_ITEM_CREATE_TFN 425
#define GEN_ITEM_CREATE_BIND 426
#define GEN_ITEM_RESIZE_STMTS_AND_SIGS 427
#define GEN_ITEM_ASSIGN_IDS 428
#define GEN_ITEM_DB_WRITE 429
#define GEN_ITEM_DB_WRITE_EXPR_TREE 430
#define GEN_ITEM_CONNECT 431
#define GEN_ITEM_RESOLVE 432
#define GEN_ITEM_BIND 433
#define GENERATE_RESOLVE 434
#define GENERATE_REMOVE_STMT_HELPER 435
#define GENERATE_REMOVE_STMT 436
#define GEN_ITEM_DEALLOC 437
#define GENERATOR_DISPLAY 438
#define GENERATOR_SORT_FUNIT_BY_FILENAME 439
#define GENERATOR_SET_NEXT_FUNIT 440
#define GENERATOR_DEALLOC_FNAME_LIST 441
#define GENERATOR_OUTPUT_FUNIT 442
#define GENERATOR_OUTPUT 443
#define GENERATOR_INIT_FUNIT 444
#define GENERATOR_PREPEND_TO_WORK_CODE 445
#define GENERATOR_ADD_TO_WORK_CODE 446
#define GENERATOR_FLUSH_WORK_CODE1 447
#define GENERATOR_ADD_TO_HOLD_CODE 448
#define GENERATOR_FLUSH_HOLD_CODE1 449
#define GENERATOR_FLUSH_EVENT_COMB 450
#define GENERATOR_FLUSH_EVENT_COMBS1 451
#define GENERATOR_FLUSH_ALL1 452
#define GENERATOR_FIND_STATEMENT 453
#define GENERATOR_FIND_CASE_STATEMENT 454
#define GENERATOR_INSERT_LINE_COV 455
#define GENERATOR_INSERT_EVENT_COMB_COV 456
#define GENERATOR_INSERT_UNARY_COMB_COV 457
#define GENERATOR_INSERT_AND_COMB_COV 458
#define GENERATOR_GEN_MSB 459
#define GENERATOR_CREATE_RHS 460
#define GENERATOR_CREATE_SUBEXP 461
#define GENERATOR_CONCAT_CODE 462
#define GENERATOR_CREATE_EXP 463
#define GENERATOR_INSERT_SUBEXP 464
#define GENERATOR_INSERT_COMB_COV_HELPER 465
#define GENERATOR_INSERT_COMB_COV 466
#define GENERATOR_INSERT_CASE_COMB_COV 467
#define GENERATOR_INSERT_FSM_COVS 468
#define SCORE_ADD_ARGS 469
#define INFO_SET_VECTOR_ELEM_SIZE 470
#define INFO_DB_WRITE 471
#define INFO_DB_READ 472
#define ARGS_DB_READ 473
#define MESSAGE_DB_READ 474
#define MERGED_CDD_DB_READ 475
#define INFO_DEALLOC 476
#define INSTANCE_DISPLAY_TREE_HELPER 477
#define INSTANCE_DISPLAY_TREE 478
#define INSTANCE_CREATE 479
#define INSTANCE_GEN_SCOPE 480
#define INSTANCE_COMPARE 481
#define INSTANCE_FIND_SCOPE 482
#define INSTANCE_FIND_BY_FUNIT 483
#define INSTANCE_FIND_BY_FUNIT_NAME_IF_ONE_HELPER 484
#define INSTANCE_FIND_BY_FUNIT_NAME_IF_ONE 485
#define INSTANCE_FIND_SIGNAL_BY_EXCLUSION_ID 486
#define INSTANCE_FIND_EXPRESSION_BY_EXCLUSION_ID 487
#define INSTANCE_FIND_FSM_ARC_INDEX_BY_EXCLUSION_ID 488
#define INSTANCE_ADD_CHILD 489
#define INSTANCE_COPY 490
#define INSTANCE_PARSE_ADD 491
#define INSTANCE_RESOLVE_INST 492
#define INSTANCE_RESOLVE_HELPER 493
#define INSTANCE_RESOLVE 494
#define INSTANCE_READ_ADD 495
#define INSTANCE_MERGE 496
#define INSTANCE_GET_LEADING_HIERARCHY 497
#define INSTANCE_MARK_LHIER_DIFFS 498
#define INSTANCE_MERGE_TWO_TREES 499
#define INSTANCE_DB_WRITE 500
#define INSTANCE_ONLY_DB_READ 501
#define INSTANCE_ONLY_DB_MERGE 502
#define INSTANCE_REMOVE_STMT_BLKS_CALLING_STMT 503
#define INSTANCE_REMOVE_PARMS_WITH_EXPR 504
#define INSTANCE_DEALLOC_SINGLE 505
#define INSTANCE_OUTPUT_DUMPVARS 506
#define INSTANCE_DEALLOC_TREE 507
#define INSTANCE_DEALLOC 508
#define STMT_ITER_RESET 509
#define STMT_ITER_COPY 510
#define STMT_ITER_NEXT 511
#define STMT_ITER_REVERSE 512
#define STMT_ITER_FIND_HEAD 513
#define STMT_ITER_GET_NEXT_IN_ORDER 514
#define STMT_ITER_GET_LINE_BEFORE 515
#define LEXER_KEYWORD_1995_CODE 516
#define LEXER_KEYWORD_2001_CODE 517
#define LEXER_KEYWORD_SV_CODE 518
#define LEXER_KEYWORD_SYS_1995_CODE 519
#define LEXER_KEYWORD_SYS_2001_CODE 520
#define LEXER_KEYWORD_SYS_SV_CODE 521
#define LEXER_CSTRING_A 522
#define LEXER_KEYWORD_A 523
#define LEXER_ESCAPED_NAME 524
#define LEXER_SYSTEM_CALL 525
#define LINE_DIRECTIVE 526
#define LINE_DIRECTIVE2 527
#define PROCESS_TIMESCALE_TOKEN 528
#define PROCESS_TIMESCALE 529
#define LEXER_YYWRAP 530
#define RESET_LEXER 531
#define RESET_LEXER_FOR_GENERATION 532
#define CHECK_FOR_PRAGMA 533
#define LINE_GET_STATS 534
#define LINE_COLLECT 535
#define LINE_GET_FUNIT_SUMMARY 536
#define LINE_GET_INST_SUMMARY 537
#define LINE_DISPLAY_INSTANCE_SUMMARY 538
#define LINE_INSTANCE_SUMMARY 539
#define LINE_DISPLAY_FUNIT_SUMMARY 540
#define LINE_FUNIT_SUMMARY 541
#define LINE_DISPLAY_VERBOSE 542
#define LINE_INSTANCE_VERBOSE 543
#define LINE_FUNIT_VERBOSE 544
#define LINE_REPORT 545
#define STR_LINK_ADD 546
#define STMT_LINK_ADD_HEAD 547
#define STMT_LINK_ADD_TAIL 548
#define STMT_LINK_MERGE 549
#define EXP_LINK_ADD 550
#define SIG_LINK_ADD 551
#define FSM_LINK_ADD 552
#define FUNIT_LINK_ADD 553
#define GITEM_LINK_ADD 554
#define INST_LINK_ADD 555
#define STR_LINK_FIND 556
#define STMT_LINK_FIND 557
#define EXP_LINK_FIND 558
#define SIG_LINK_FIND 559
#define FSM_LINK_FIND 560
#define FUNIT_LINK_FIND 561
#define GITEM_LINK_FIND 562
#define INST_LINK_FIND_BY_SCOPE 563
#define INST_LINK_FIND_BY_FUNIT 564
#define STR_LINK_REMOVE 565
#define EXP_LINK_REMOVE 566
#define GITEM_LINK_REMOVE 567
#define FUNIT_LINK_REMOVE 568
#define STR_LINK_DELETE_LIST 569
#define STMT_LINK_UNLINK 570
#define STMT_LINK_DELETE_LIST 571
#define EXP_LINK_DELETE_LIST 572
#define SIG_LINK_DELETE_LIST 573
#define FSM_LINK_DELETE_LIST 574
#define FUNIT_LINK_DELETE_LIST 575
#define GITEM_LINK_DELETE_LIST 576
#define INST_LINK_DELETE_LIST 577
#define VCDID 578
#define VCD_CALLBACK 579
#define LXT_PARSE 580
#define LXT2_RD_EXPAND_INTEGER_TO_BITS 581
#define LXT2_RD_EXPAND_BITS_TO_INTEGER 582
#define LXT2_RD_ITER_RADIX 583
#define LXT2_RD_ITER_RADIX0 584
#define LXT2_RD_BUILD_RADIX 585
#define LXT2_RD_REGENERATE_PROCESS_MASK 586
#define LXT2_RD_PROCESS_BLOCK 587
#define LXT2_RD_INIT 588
#define LXT2_RD_CLOSE 589
#define LXT2_RD_GET_FACNAME 590
#define LXT2_RD_ITER_BLOCKS 591
#define LXT2_RD_LIMIT_TIME_RANGE 592
#define LXT2_RD_UNLIMIT_TIME_RANGE 593
#define MEMORY_GET_STAT 594
#define MEMORY_GET_STATS 595
#define MEMORY_GET_FUNIT_SUMMARY 596
#define MEMORY_GET_INST_SUMMARY 597
#define MEMORY_CREATE_PDIM_BIT_ARRAY 598
#define MEMORY_GET_MEM_COVERAGE 599
#define MEMORY_GET_COVERAGE 600
#define MEMORY_COLLECT 601
#define MEMORY_DISPLAY_TOGGLE_INSTANCE_SUMMARY 602
#define MEMORY_TOGGLE_INSTANCE_SUMMARY 603
#define MEMORY_DISPLAY_AE_INSTANCE_SUMMARY 604
#define MEMORY_AE_INSTANCE_SUMMARY 605
#define MEMORY_DISPLAY_TOGGLE_FUNIT_SUMMARY 606
#define MEMORY_TOGGLE_FUNIT_SUMMARY 607
#define MEMORY_DISPLAY_AE_FUNIT_SUMMARY 608
#define MEMORY_AE_FUNIT_SUMMARY 609
#define MEMORY_DISPLAY_MEMORY 610
#define MEMORY_DISPLAY_VERBOSE 611
#define MEMORY_INSTANCE_VERBOSE 612
#define MEMORY_FUNIT_VERBOSE 613
#define MEMORY_REPORT 614
#define COMMAND_MERGE 615
#define OBFUSCATE_SET_MODE 616
#define OBFUSCATE_NAME 617
#define OBFUSCATE_DEALLOC 618
#define OVL_IS_ASSERTION_NAME 619
#define OVL_IS_ASSERTION_MODULE 620
#define OVL_IS_COVERAGE_POINT 621
#define OVL_ADD_ASSERTIONS_TO_NO_SCORE_LIST 622
#define OVL_GET_FUNIT_STATS 623
#define OVL_GET_COVERAGE_POINT 624
#define OVL_DISPLAY_VERBOSE 625
#define OVL_COLLECT 626
#define OVL_GET_COVERAGE 627
#define MOD_PARM_FIND 628
#define MOD_PARM_FIND_EXPR_AND_REMOVE 629
#define MOD_PARM_ADD 630
#define INST_PARM_FIND 631
#define INST_PARM_ADD 632
#define INST_PARM_ADD_GENVAR 633
#define INST_PARM_BIND 634
#define DEFPARAM_ADD 635
#define DEFPARAM_DEALLOC 636
#define PARAM_FIND_AND_SET_EXPR_VALUE 637
#define PARAM_SET_SIG_SIZE 638
#define PARAM_SIZE_FUNCTION 639
#define PARAM_EXPR_EVAL 640
#define PARAM_HAS_OVERRIDE 641
#define PARAM_HAS_DEFPARAM 642
#define PARAM_RESOLVE_DECLARED 643
#define PARAM_RESOLVE_OVERRIDE 644
#define PARAM_RESOLVE 645
#define PARAM_DB_WRITE 646
#define MOD_PARM_DEALLOC 647
#define INST_PARM_DEALLOC 648
#define PARSE_READLINE 649
#define PARSE_DESIGN 650
#define PARSE_AND_SCORE_DUMPFILE 651
#define PARSER_STATIC_EXPR_PRIMARY_A 652
#define PARSER_STATIC_EXPR_PRIMARY_B 653
#define PARSER_EXPRESSION_LIST_A 654
#define PARSER_EXPRESSION_LIST_B 655
#define PARSER_EXPRESSION_LIST_C 656
#define PARSER_EXPRESSION_LIST_D 657
#define PARSER_IDENTIFIER_A 658
#define PARSER_GENERATE_CASE_ITEM_A 659
#define PARSER_GENERATE_CASE_ITEM_B 660
#define PARSER_GENERATE_CASE_ITEM_C 661
#define PARSER_STATEMENT_BEGIN_A 662
#define PARSER_STATEMENT_FORK_A 663
#define PARSER_STATEMENT_FOR_A 664
#define PARSER_CASE_ITEM_A 665
#define PARSER_CASE_ITEM_B 666
#define PARSER_CASE_ITEM_C 667
#define PARSER_DELAY_VALUE_A 668
#define PARSER_DELAY_VALUE_B 669
#define PARSER_PARAMETER_VALUE_BYNAME_A 670
#define PARSER_GATE_INSTANCE_A 671
#define PARSER_GATE_INSTANCE_B 672
#define PARSER_GATE_INSTANCE_C 673
#define PARSER_GATE_INSTANCE_D 674
#define PARSER_LIST_OF_NAMES_A 675
#define PARSER_LIST_OF_NAMES_B 676
#define PARSER_CHECK_PSTAR 677
#define PARSER_CHECK_ATTRIBUTE 678
#define PARSER_CREATE_ATTR_LIST 679
#define PARSER_CREATE_ATTR 680
#define PARSER_CREATE_TASK_DECL 681
#define PARSER_CREATE_TASK_BODY 682
#define PARSER_CREATE_FUNCTION_DECL 683
#define PARSER_CREATE_FUNCTION_BODY 684
#define PARSER_END_TASK_FUNCTION 685
#define PARSER_CREATE_PORT 686
#define PARSER_HANDLE_INLINE_PORT_ERROR 687
#define PARSER_CREATE_SIMPLE_NUMBER 688
#define PARSER_CREATE_COMPLEX_NUMBER 689
#define PARSER_APPEND_SE_PORT_LIST 690
#define PARSER_CREATE_SE_PORT_LIST 691
#define PARSER_CREATE_UNARY_SE 692
#define PARSER_CREATE_SYSCALL_SE 693
#define PARSER_CREATE_UNARY_EXP 694
#define PARSER_CREATE_BINARY_EXP 695
#define PARSER_CREATE_OP_AND_ASSIGN_EXP 696
#define PARSER_CREATE_SYSCALL_EXP 697
#define PARSER_CREATE_SYSCALL_W_PARAMS_EXP 698
#define PARSER_CREATE_OP_AND_ASSIGN_W_DIM_EXP 699
#define VLERROR 700
#define VLWARN 701
#define PARSER_DEALLOC_SIG_RANGE 702
#define PARSER_COPY_CURR_RANGE 703
#define PARSER_COPY_RANGE_TO_CURR_RANGE 704
#define PARSER_EXPLICITLY_SET_CURR_RANGE 705
#define PARSER_IMPLICITLY_SET_CURR_RANGE 706
#define PARSER_CHECK_GENERATION 707
#define PERF_GEN_STATS 708
#define PERF_OUTPUT_MOD_STATS 709
#define PERF_OUTPUT_INST_REPORT_HELPER 710
#define PERF_OUTPUT_INST_REPORT 711
#define DEF_LOOKUP 712
#define IS_DEFINED 713
#define DEF_MATCH 714
#define DEF_START 715
#define DEFINE_MACRO 716
#define DO_DEFINE 717
#define DEF_IS_DONE 718
#define DEF_FINISH 719
#define DEF_UNDEFINE 720
#define INCLUDE_FILENAME 721
#define DO_INCLUDE 722
#define YYWRAP 723
#define RESET_PPLEXER 724
#define RACE_BLK_CREATE 725
#define RACE_FIND_HEAD_STATEMENT_CONTAINING_STATEMENT_HELPER 726
#define RACE_FIND_HEAD_STATEMENT_CONTAINING_STATEMENT 727
#define RACE_GET_HEAD_STATEMENT 728
#define RACE_FIND_HEAD_STATEMENT 729
#define RACE_CALC_STMT_BLK_TYPE 730
#define RACE_CALC_EXPR_ASSIGNMENT 731
#define RACE_CALC_ASSIGNMENTS 732
#define RACE_HANDLE_RACE_CONDITION 733
#define RACE_CHECK_ASSIGNMENT_TYPES 734
#define RACE_CHECK_ONE_BLOCK_ASSIGNMENT 735
#define RACE_CHECK_RACE_COUNT 736
#define RACE_CHECK_MODULES 737
#define RACE_DB_WRITE 738
#define RACE_DB_READ 739
#define RACE_GET_STATS 740
#define RACE_REPORT_SUMMARY 741
#define RACE_REPORT_VERBOSE 742
#define RACE_REPORT 743
#define RACE_COLLECT_LINES 744
#define RACE_BLK_DELETE_LIST 745
#define RANK_CREATE_COMP_CDD_COV 746
#define RANK_DEALLOC_COMP_CDD_COV 747
#define RANK_CHECK_INDEX 748
#define RANK_GATHER_SIGNAL_COV 749
#define RANK_GATHER_COMB_COV 750
#define RANK_GATHER_EXPRESSION_COV 751
#define RANK_GATHER_FSM_COV 752
#define RANK_CALC_NUM_CPS 753
#define RANK_GATHER_COMP_CDD_COV 754
#define RANK_READ_CDD 755
#define RANK_SELECTED_CDD_COV 756
#define RANK_PERFORM_WEIGHTED_SELECTION 757
#define RANK_PERFORM_GREEDY_SORT 758
#define RANK_COUNT_CPS 759
#define RANK_PERFORM 760
#define RANK_OUTPUT 761
#define COMMAND_RANK 762
#define REENTRANT_COUNT_AFU_BITS 763
#define REENTRANT_STORE_DATA_BITS 764
#define REENTRANT_RESTORE_DATA_BITS 765
#define REENTRANT_CREATE 766
#define REENTRANT_DEALLOC 767
#define REPORT_PARSE_METRICS 768
#define REPORT_PARSE_ARGS 769
#define REPORT_GATHER_INSTANCE_STATS 770
#define REPORT_GATHER_FUNIT_STATS 771
#define REPORT_PRINT_HEADER 772
#define REPORT_GENERATE 773
#define REPORT_READ_CDD_AND_READY 774
#define REPORT_CLOSE_CDD 775
#define REPORT_SAVE_CDD 776
#define REPORT_FORMAT_EXCLUSION_REASON 777
#define REPORT_OUTPUT_EXCLUSION_REASON 778
#define COMMAND_REPORT 779
#define SCOPE_FIND_FUNIT_FROM_SCOPE 780
#define SCOPE_FIND_PARAM 781
#define SCOPE_FIND_SIGNAL 782
#define SCOPE_FIND_TASK_FUNCTION_NAMEDBLOCK 783
#define SCOPE_GET_PARENT_FUNIT 784
#define SCOPE_GET_PARENT_MODULE 785
#define SCORE_GENERATE_TOP_VPI_MODULE 786
#define SCORE_GENERATE_TOP_DUMPVARS_MODULE 787
#define SCORE_GENERATE_PLI_TAB_FILE 788
#define SCORE_PARSE_DEFINE 789
#define SCORE_PARSE_ARGS 790
#define COMMAND_SCORE 791
#define SEARCH_INIT 792
#define SEARCH_ADD_INCLUDE_PATH 793
#define SEARCH_ADD_DIRECTORY_PATH 794
#define SEARCH_ADD_FILE 795
#define SEARCH_ADD_NO_SCORE_FUNIT 796
#define SEARCH_ADD_EXTENSIONS 797
#define SEARCH_FREE_LISTS 798
#define SIM_CURRENT_THREAD 799
#define SIM_THREAD_POP_HEAD 800
#define SIM_THREAD_INSERT_INTO_DELAY_QUEUE 801
#define SIM_THREAD_PUSH 802
#define SIM_EXPR_CHANGED 803
#define SIM_CREATE_THREAD 804
#define SIM_ADD_THREAD 805
#define SIM_KILL_THREAD 806
#define SIM_KILL_THREAD_WITH_FUNIT 807
#define SIM_ADD_STATICS 808
#define SIM_EXPRESSION 809
#define SIM_THREAD 810
#define SIM_SIMULATE 811
#define SIM_INITIALIZE 812
#define SIM_STOP 813
#define SIM_FINISH 814
#define SIM_DEALLOC 815
#define STATISTIC_CREATE 816
#define STATISTIC_IS_EMPTY 817
#define STATISTIC_DEALLOC 818
#define STATEMENT_CREATE 819
#define STATEMENT_QUEUE_ADD 820
#define STATEMENT_QUEUE_COMPARE 821
#define STATEMENT_SIZE_ELEMENTS 822
#define STATEMENT_DB_WRITE 823
#define STATEMENT_DB_WRITE_TREE 824
#define STATEMENT_DB_WRITE_EXPR_TREE 825
#define STATEMENT_DB_READ 826
#define STATEMENT_ASSIGN_EXPR_IDS 827
#define STATEMENT_CONNECT 828
#define STATEMENT_GET_LAST_LINE_HELPER 829
#define STATEMENT_GET_LAST_LINE 830
#define STATEMENT_FIND_RHS_SIGS 831
#define STATEMENT_FIND_HEAD_STATEMENT 832
#define STATEMENT_FIND_STATEMENT 833
#define STATEMENT_FIND_STATEMENT_BY_POSITION 834
#define STATEMENT_CONTAINS_EXPR_CALLING_STMT 835
#define STATEMENT_DEALLOC_RECURSIVE 836
#define STATEMENT_DEALLOC 837
#define STATIC_EXPR_GEN_UNARY 838
#define STATIC_EXPR_GEN 839
#define STATIC_EXPR_CALC_LSB_AND_WIDTH_PRE 840
#define STATIC_EXPR_CALC_LSB_AND_WIDTH_POST 841
#define STATIC_EXPR_DEALLOC 842
#define STMT_BLK_ADD_TO_REMOVE_LIST 843
#define STMT_BLK_REMOVE 844
#define STMT_BLK_SPECIFY_REMOVAL_REASON 845
#define STRUCT_UNION_LENGTH 846
#define STRUCT_UNION_ADD_MEMBER 847
#define STRUCT_UNION_ADD_MEMBER_VOID 848
#define STRUCT_UNION_ADD_MEMBER_SIG 849
#define STRUCT_UNION_ADD_MEMBER_TYPEDEF 850
#define STRUCT_UNION_ADD_MEMBER_ENUM 851
#define STRUCT_UNION_ADD_MEMBER_STRUCT_UNION 852
#define STRUCT_UNION_CREATE 853
#define STRUCT_UNION_MEMBER_DEALLOC 854
#define STRUCT_UNION_DEALLOC 855
#define STRUCT_UNION_DEALLOC_LIST 856
#define SYMTABLE_ADD_SYM_SIG 857
#define SYMTABLE_ADD_SYM_EXP 858
#define SYMTABLE_ADD_SYM_FSM 859
#define SYMTABLE_INIT 860
#define SYMTABLE_CREATE 861
#define SYMTABLE_GET_TABLE 862
#define SYMTABLE_ADD_SIGNAL 863
#define SYMTABLE_ADD_EXPRESSION 864
#define SYMTABLE_ADD_FSM 865
#define SYMTABLE_SET_VALUE 866
#define SYMTABLE_ASSIGN 867
#define SYMTABLE_DEALLOC 868
#define SYS_TASK_UNIFORM 869
#define SYS_TASK_RTL_DIST_UNIFORM 870
#define SYS_TASK_SRANDOM 871
#define SYS_TASK_RANDOM 872
#define SYS_TASK_URANDOM 873
#define SYS_TASK_URANDOM_RANGE 874
#define SYS_TASK_REALTOBITS 875
#define SYS_TASK_BITSTOREAL 876
#define SYS_TASK_SHORTREALTOBITS 877
#define SYS_TASK_BITSTOSHORTREAL 878
#define SYS_TASK_ITOR 879
#define SYS_TASK_RTOI 880
#define SYS_TASK_STORE_PLUSARGS 881
#define SYS_TASK_TEST_PLUSARG 882
#define SYS_TASK_VALUE_PLUSARGS 883
#define SYS_TASK_DEALLOC 884
#define TCL_FUNC_GET_RACE_REASON_MSGS 885
#define TCL_FUNC_GET_FUNIT_LIST 886
#define TCL_FUNC_GET_INSTANCES 887
#define TCL_FUNC_GET_INSTANCE_LIST 888
#define TCL_FUNC_IS_FUNIT 889
#define TCL_FUNC_GET_FUNIT 890
#define TCL_FUNC_GET_INST 891
#define TCL_FUNC_GET_FUNIT_NAME 892
#define TCL_FUNC_GET_FILENAME 893
#define TCL_FUNC_INST_SCOPE 894
#define TCL_FUNC_GET_FUNIT_START_AND_END 895
#define TCL_FUNC_COLLECT_UNCOVERED_LINES 896
#define TCL_FUNC_COLLECT_COVERED_LINES 897
#define TCL_FUNC_COLLECT_RACE_LINES 898
#define TCL_FUNC_COLLECT_UNCOVERED_TOGGLES 899
#define TCL_FUNC_COLLECT_COVERED_TOGGLES 900
#define TCL_FUNC_COLLECT_UNCOVERED_MEMORIES 901
#define TCL_FUNC_COLLECT_COVERED_MEMORIES 902
#define TCL_FUNC_GET_TOGGLE_COVERAGE 903
#define TCL_FUNC_GET_MEMORY_COVERAGE 904
#define TCL_FUNC_COLLECT_UNCOVERED_COMBS 905
#define TCL_FUNC_COLLECT_COVERED_COMBS 906
#define TCL_FUNC_GET_COMB_EXPRESSION 907
#define TCL_FUNC_GET_COMB_COVERAGE 908
#define TCL_FUNC_COLLECT_UNCOVERED_FSMS 909
#define TCL_FUNC_COLLECT_COVERED_FSMS 910
#define TCL_FUNC_GET_FSM_COVERAGE 911
#define TCL_FUNC_COLLECT_UNCOVERED_ASSERTIONS 912
#define TCL_FUNC_COLLECT_COVERED_ASSERTIONS 913
#define TCL_FUNC_GET_ASSERT_COVERAGE 914
#define TCL_FUNC_OPEN_CDD 915
#define TCL_FUNC_CLOSE_CDD 916
#define TCL_FUNC_SAVE_CDD 917
#define TCL_FUNC_MERGE_CDD 918
#define TCL_FUNC_GET_LINE_SUMMARY 919
#define TCL_FUNC_GET_TOGGLE_SUMMARY 920
#define TCL_FUNC_GET_MEMORY_SUMMARY 921
#define TCL_FUNC_GET_COMB_SUMMARY 922
#define TCL_FUNC_GET_FSM_SUMMARY 923
#define TCL_FUNC_GET_ASSERT_SUMMARY 924
#define TCL_FUNC_PREPROCESS_VERILOG 925
#define TCL_FUNC_GET_SCORE_PATH 926
#define TCL_FUNC_GET_INCLUDE_PATHNAME 927
#define TCL_FUNC_GET_GENERATION 928
#define TCL_FUNC_SET_LINE_EXCLUDE 929
#define TCL_FUNC_SET_TOGGLE_EXCLUDE 930
#define TCL_FUNC_SET_MEMORY_EXCLUDE 931
#define TCL_FUNC_SET_COMB_EXCLUDE 932
#define TCL_FUNC_FSM_EXCLUDE 933
#define TCL_FUNC_SET_ASSERT_EXCLUDE 934
#define TCL_FUNC_GENERATE_REPORT 935
#define TCL_FUNC_INITIALIZE 936
#define TOGGLE_GET_STATS 937
#define TOGGLE_COLLECT 938
#define TOGGLE_GET_COVERAGE 939
#define TOGGLE_GET_FUNIT_SUMMARY 940
#define TOGGLE_GET_INST_SUMMARY 941
#define TOGGLE_DISPLAY_INSTANCE_SUMMARY 942
#define TOGGLE_INSTANCE_SUMMARY 943
#define TOGGLE_DISPLAY_FUNIT_SUMMARY 944
#define TOGGLE_FUNIT_SUMMARY 945
#define TOGGLE_DISPLAY_VERBOSE 946
#define TOGGLE_INSTANCE_VERBOSE 947
#define TOGGLE_FUNIT_VERBOSE 948
#define TOGGLE_REPORT 949
#define TREE_ADD 950
#define TREE_FIND 951
#define TREE_REMOVE 952
#define TREE_DEALLOC 953
#define CHECK_OPTION_VALUE 954
#define IS_VARIABLE 955
#define IS_FUNC_UNIT 956
#define IS_LEGAL_FILENAME 957
#define GET_BASENAME 958
#define GET_DIRNAME 959
#define GET_ABSOLUTE_PATH 960
#define GET_RELATIVE_PATH 961
#define DIRECTORY_EXISTS 962
#define DIRECTORY_LOAD 963
#define FILE_EXISTS 964
#define UTIL_READLINE 965
#define GET_QUOTED_STRING 966
#define SUBSTITUTE_ENV_VARS 967
#define SCOPE_EXTRACT_FRONT 968
#define SCOPE_EXTRACT_BACK 969
#define SCOPE_EXTRACT_SCOPE 970
#define SCOPE_GEN_PRINTABLE 971
#define SCOPE_COMPARE 972
#define SCOPE_LOCAL 973
#define CONVERT_FILE_TO_MODULE 974
#define GET_NEXT_VFILE 975
#define GEN_SPACE 976
#define REMOVE_UNDERSCORES 977
#define GET_FUNIT_TYPE 978
#define CALC_MISS_PERCENT 979
#define READ_COMMAND_FILE 980
#define VCD_CALC_INDEX 981
#define VCD_PARSE_DEF_IGNORE 982
#define VCD_PARSE_DEF_VAR 983
#define VCD_PARSE_DEF_SCOPE 984
#define VCD_PARSE_DEF 985
#define VCD_PARSE_SIM_VECTOR 986
#define VCD_PARSE_SIM_REAL 987
#define VCD_PARSE_SIM_IGNORE 988
#define VCD_PARSE_SIM 989
#define VCD_PARSE 990
#define VECTOR_INIT 991
#define VECTOR_INT_R64 992
#define VECTOR_INT_R32 993
#define VECTOR_CREATE 994
#define VECTOR_COPY 995
#define VECTOR_COPY_RANGE 996
#define VECTOR_CLONE 997
#define VECTOR_DB_WRITE 998
#define VECTOR_DB_READ 999
#define VECTOR_DB_MERGE 1000
#define VECTOR_MERGE 1001
#define VECTOR_GET_EVAL_A 1002
#define VECTOR_GET_EVAL_B 1003
#define VECTOR_GET_EVAL_C 1004
#define VECTOR_GET_EVAL_D 1005
#define VECTOR_GET_EVAL_AB_COUNT 1006
#define VECTOR_GET_EVAL_ABC_COUNT 1007
#define VECTOR_GET_EVAL_ABCD_COUNT 1008
#define VECTOR_GET_TOGGLE01_ULONG 1009
#define VECTOR_GET_TOGGLE10_ULONG 1010
#define VECTOR_DISPLAY_TOGGLE01_ULONG 1011
#define VECTOR_DISPLAY_TOGGLE10_ULONG 1012
#define VECTOR_TOGGLE_COUNT 1013
#define VECTOR_MEM_RW_COUNT 1014
#define VECTOR_SET_ASSIGNED 1015
#define VECTOR_SET_COVERAGE_AND_ASSIGN 1016
#define VECTOR_GET_SIGN_EXTEND_VECTOR_ULONG 1017
#define VECTOR_SIGN_EXTEND_ULONG 1018
#define VECTOR_LSHIFT_ULONG 1019
#define VECTOR_RSHIFT_ULONG 1020
#define VECTOR_SET_VALUE 1021
#define VECTOR_PART_SELECT_PULL 1022
#define VECTOR_PART_SELECT_PUSH 1023
#define VECTOR_SET_UNARY_EVALS 1024
#define VECTOR_SET_AND_COMB_EVALS 1025
#define VECTOR_SET_OR_COMB_EVALS 1026
#define VECTOR_SET_OTHER_COMB_EVALS 1027
#define VECTOR_IS_UKNOWN 1028
#define VECTOR_IS_NOT_ZERO 1029
#define VECTOR_SET_TO_X 1030
#define VECTOR_TO_INT 1031
#define VECTOR_TO_UINT64 1032
#define VECTOR_TO_REAL64 1033
#define VECTOR_TO_SIM_TIME 1034
#define VECTOR_FROM_INT 1035
#define VECTOR_FROM_UINT64 1036
#define VECTOR_FROM_REAL64 1037
#define VECTOR_SET_STATIC 1038
#define VECTOR_TO_STRING 1039
#define VECTOR_FROM_STRING_FIXED 1040
#define VECTOR_FROM_STRING 1041
#define VECTOR_VCD_ASSIGN 1042
#define VECTOR_VCD_ASSIGN2 1043
#define VECTOR_BITWISE_AND_OP 1044
#define VECTOR_BITWISE_NAND_OP 1045
#define VECTOR_BITWISE_OR_OP 1046
#define VECTOR_BITWISE_NOR_OP 1047
#define VECTOR_BITWISE_XOR_OP 1048
#define VECTOR_BITWISE_NXOR_OP 1049
#define VECTOR_OP_LT 1050
#define VECTOR_OP_LE 1051
#define VECTOR_OP_GT 1052
#define VECTOR_OP_GE 1053
#define VECTOR_OP_EQ 1054
#define VECTOR_CEQ_ULONG 1055
#define VECTOR_OP_CEQ 1056
#define VECTOR_OP_CXEQ 1057
#define VECTOR_OP_CZEQ 1058
#define VECTOR_OP_NE 1059
#define VECTOR_OP_CNE 1060
#define VECTOR_OP_LOR 1061
#define VECTOR_OP_LAND 1062
#define VECTOR_OP_LSHIFT 1063
#define VECTOR_OP_RSHIFT 1064
#define VECTOR_OP_ARSHIFT 1065
#define VECTOR_OP_ADD 1066
#define VECTOR_OP_NEGATE 1067
#define VECTOR_OP_SUBTRACT 1068
#define VECTOR_OP_MULTIPLY 1069
#define VECTOR_OP_DIVIDE 1070
#define VECTOR_OP_MODULUS 1071
#define VECTOR_OP_INC 1072
#define VECTOR_OP_DEC 1073
#define VECTOR_UNARY_INV 1074
#define VECTOR_UNARY_AND 1075
#define VECTOR_UNARY_NAND 1076
#define VECTOR_UNARY_OR 1077
#define VECTOR_UNARY_NOR 1078
#define VECTOR_UNARY_XOR 1079
#define VECTOR_UNARY_NXOR 1080
#define VECTOR_UNARY_NOT 1081
#define VECTOR_OP_EXPAND 1082
#define VECTOR_OP_LIST 1083
#define VECTOR_DEALLOC_VALUE 1084
#define VECTOR_DEALLOC 1085
#define SYM_VALUE_STORE 1086
#define ADD_SYM_VALUES_TO_SIM 1087
#define COVERED_VALUE_CHANGE_BIN 1088
#define COVERED_VALUE_CHANGE_REAL 1089
#define COVERED_END_OF_SIM 1090
#define COVERED_CB_ERROR_HANDLER 1091
#define GEN_NEXT_SYMBOL 1092
#define COVERED_CREATE_VALUE_CHANGE_CB 1093
#define COVERED_PARSE_TASK_FUNC 1094
#define COVERED_PARSE_SIGNALS 1095
#define COVERED_PARSE_INSTANCE 1096
#define COVERED_SIM_CALLTF 1097
#define COVERED_REGISTER 1098
#define VSIGNAL_INIT 1099
#define VSIGNAL_CREATE 1100
#define VSIGNAL_CREATE_VEC 1101
#define VSIGNAL_DUPLICATE 1102
#define VSIGNAL_DB_WRITE 1103
#define VSIGNAL_DB_READ 1104
#define VSIGNAL_DB_MERGE 1105
#define VSIGNAL_MERGE 1106
#define VSIGNAL_PROPAGATE 1107
#define VSIGNAL_VCD_ASSIGN 1108
#define VSIGNAL_ADD_EXPRESSION 1109
#define VSIGNAL_FROM_STRING 1110
#define VSIGNAL_CALC_WIDTH_FOR_EXPR 1111
#define VSIGNAL_CALC_LSB_FOR_EXPR 1112
#define VSIGNAL_DEALLOC 1113

extern profiler profiles[NUM_PROFILES];
#endif

extern unsigned int profile_index;

#endif

