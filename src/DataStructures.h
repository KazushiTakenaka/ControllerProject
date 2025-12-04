#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

// 送信するデータの構造体を定義します
struct SaneDataPacket {
  int slideVal1, slideVal2;
  int sld_sw1_1, sld_sw1_2, sld_sw2_1, sld_sw2_2;
  int sld_sw3_1, sld_sw3_2, sld_sw4_1, sld_sw4_2;
  int sw1, sw2, sw3, sw4, sw5, sw6, sw7, sw8;
};

// 受信するデータの構造体を定義します
struct ReceivedDataPacket {
  int val1, val2, val3, val4, val5;
};

#endif // DATA_STRUCTURES_H
