// license:BSD-3-Clause
// copyright-holders:hap
// modded by bzhxx
/*

  Sharp SM500 MCU family cores

*/

#ifndef _SM500_H_
#define _SM500_H_

#pragma once

#include "gw_type_defs.h"

// device-level SM5A/SM500 API
void sm500_device_start(void);
void sm500_device_reset(void);
void sm500_execute_run(void);

void sm500_get_opcode_param(void);
void sm500_div_timer(int nb_inst);
void sm500_update_segments_state(void);

void sm500_reset_vector(void);
void sm500_wakeup_vector(void);
bool sm500_wake_me_up(void);

void shift_w(void);
u8 get_digit(void);
void set_su(u8 su);
u8 get_su(void);
int get_trs_field(void);

//variation SM5A
void sm5a_device_start(void);
void sm5a_device_reset(void);
void sm5a_execute_run(void);

// opcode handlers for SM5A,SM500
// other opcodes are also used from SM510

/*Start of list */

void sm500_op_lax(void);
void sm500_op_adx(void);
void sm500_op_rm(void);
void sm500_op_sm(void);
void sm500_op_exc(void);
void sm500_op_exci(void);
void sm500_op_lda(void);
void sm500_op_excd(void);
void sm500_op_tmi(void);
void sm500_op_skip(void);
void sm500_op_atr(void);
void sm500_op_add(void);
void sm500_op_add11(void);
void sm500_op_coma(void);
void sm500_op_exbla(void);

void sm500_op_tal(void);
void sm500_op_tb(void);
void sm500_op_tc(void);
void sm500_op_tam(void);
void sm500_op_tis(void);

void sm500_op_ta0(void);
void sm500_op_tabl(void);

// custom
void sm500_op_lbl(void);

void sm500_op_rc(void);
void sm500_op_sc(void);
void sm500_op_kta(void);

void sm500_op_decb(void);

void sm500_op_rtn1(void);

void sm500_op_cend(void);
void sm500_op_dta(void);
void sm500_op_illegal(void);

/*End of OP list : SM510 */

// SM500 specific instruction or deviation
void sm500_op_lb(void);
void sm500_op_incb(void); //SM510
void sm500_op_sbm(void);  //SM510
void sm500_op_rbm(void);

void sm500_op_comcb(void);
void sm500_op_rtn0(void); //SM510
void sm500_op_ssr(void);
void sm500_op_tr(void);
void sm500_op_trs(void);

void sm500_op_atbp(void); //SM510
void sm500_op_ptw(void);  //SM510
void sm500_op_tw(void);
void sm500_op_pdtw(void);
void sm500_op_dtw(void);
void sm500_op_wr(void); //SM510
void sm500_op_ws(void); //SM510

void sm500_op_ats(void);
void sm500_op_exksa(void);
void sm500_op_exkfa(void);

void sm500_op_idiv(void); //SM510

void sm500_op_rmf(void);
void sm500_op_smf(void);
void sm500_op_comcn(void);

#endif // _SM500_H_
