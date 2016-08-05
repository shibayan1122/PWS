///////////////////////////////////////////////////////////
// pws_btn.h
///////////////////////////////////////////////////////////
#ifndef __PWS_BTN_H__
#define __PWS_BTN_H__

#define MSG_PUSH_AP_SET_BTN     "/btnmonitor/push/apset"        // AP�ݒ�{�^������
#define MSG_PUSH_REC_BTN        "/btnmonitor/push/recbtn"       // �^���{�^������
#define MSG_PUSH_PLAY_BTN       "/btnmonitor/push/playbtn"      // �Đ��{�^������
#define MSG_PUSH_VOL_UP_BTN     "/btnmonitor/push/volupbtn"     // �{�����[���A�b�v�{�^������
#define MSG_PUSH_VOL_DOWN_BTN   "/btnmonitor/push/voldownbtn"   // �{�����[���_�E���{�^������
#define MSG_PUSH_EFFECT_BTN     "/btnmonitor/push/effectbtn"    // �G�t�F�N�g�{�^������
#define MSG_PUSH_TUNING_BTN     "/btnmonitor/push/tuningbtn"    // �`���[�j���O�{�^������
#define MSG_PUSH_SHUTDOWN_BTN   "/btnmonitor/push/shutdownbtn"  // �V���b�g�_�E���{�^������

//
// �X�C�b�`�Ď�������
//
extern int btnInitialize(void);

//
// �X�C�b�`�Ď��I������
//
extern void btnFinish(void);

#endif // __PWS_BTN_H__
