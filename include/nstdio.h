#ifndef __INCLUDE_NSTDIO_H__
#define __INCLUDE_NSTDIO_H__

/**
 * @file nstdio.h
 * @breif a header file for NSTDIO
 *
 * This is NSTDIO header file.
*/

#include <ppstream.h>

#define NTCP PPSTREAM_TCP
#define NUDP PPSTREAM_UDP
#define NIBRC PPSTREAM_IBRC
#define NIBUD PPSTREAM_IBUD

typedef ppstream_networkinfo_t NET;

typedef ppstream_networkdescriptor_t ND;

typedef ppstream_handle_t NHDL;

/**
 * @JP
 * @brief ネットワークへの接続の開く
 * 
 * nopen関数は、ネットワーク情報 nt 
 * で指定されたプロセスに対して接続を行い、
 * 対応するネットワークとストリームを結びつける。
 *
 * @param nt ネットワーク情報構造体
 * @param mode ネットワークのオープン時のモード
 *      - &quot;s&quot;: サーバとして接続を待つ。
 *      - &quot;c&quot;: クライアントとして接続する。
 *      - &quot;r&quot;: サーバとして接続を待つ。接続確立後は読み込み（受信）のみ可能
 *      - &quot;w&quot;: クライアントして接続する。接続確立後は書き込み（送信）のみ可能
 * @retval 成功 ネットワークディスクリプタ 
 * @retval 失敗 NULL
*/
ND *nopen(NET *nt, char *mode);

/**
 * @JP
 * @brief ネットワークへの接続を閉じる
 *
 * nclose関数は、指定したネットワークディスクリプタに対応する接続を閉じる。
 *
 * @param nd オープンされたネットワークディスクリプタ
 * @retval なし
*/
void nclose(ND *nd);

/**
 * @JP
 * @brief 送信処理発行
 *
 * nwrite関数は、指定したネットワークディスクリプタによる接続に対して
 * 指定送信バッファを元に送信処理を行う。本関数はノンブロッキング動作を行う。
 * 受信処理の完了はnquery関数により知る。本関数を実行後、
 * 完了確認を実施前に送信領域を書き換えた場合には、受信先のデータの内容は不定。
 *
 * @param nd オープンされたネットワーウディスクリプタ
 * @param ptr 送信バッファの先頭アドレス
 * @param size 送信データサイズ
 *
 * @retval 成功 通信ハンドル
 * @retval 失敗 NULL
 */
NHDL *nwrite(ND *nd, void *ptr, size_t size);

/**
 * @JP
 * @brief 受信処理発行
 * 
 * nread関数は、指定したネットワークディスクリプタによる接続に対して
 * 指定受信バッファへのデータの受信を行う。本関数はノンブロッキング動作を行う。
 * 受信処理の完了はnquery関数により知る。本関数を実行後、
 * 完了確認を実施前に受信領域を書き換えた場合には、データ内容は不定。
 *
 * @param nd オープンされたネットワーウディスクリプタ
 * @param ptr 受信バッファの先頭アドレス
 * @param size 受信ータサイズ
 *
 * @retval 成功 通信ハンドル
 * @retval 失敗 NULL
 */
NHDL *nread(ND *nd, void *ptr, size_t size);

/**
 * @JP
 * @brief 完了確認
 *
 * nquery関数は、指定した通信ハンドルに対する送信および
 * 受信の処理の完了確認を実施する。
 * nwrite, nreadで指定されたsizeの送信、受信の完了を待つ。
 *
 * @param hdl 通信ハンドル
 *
 * @retval 通信完了 0
 * @retval 通信未完了 1
 */
int nquery(NHDL *hdl);

/**
 * @JP
 * @brief プロセス間の同期を実行
 *
 * nsync関数は、指定したネットワークディスクリプタに対応する接続間において同期を実行。
 *
 * @param nd オープンされたネットワークディスクリプタ
 * @retval なし
*/
void nsync(ND *nd);

/**
 * @JP
 * @brief 接続先プロセスのネットワーク情報の生成
 *
 * setnet関数は、指示したホストネーム hostname、
 * ポート番号 servname、通信デバイスの
 * 指定フラグ Dflagをもとにネットワーク情報を生成し、その構造体を返す。
 *
 * @param hostname ホストネームもしくはIPアドレス
 * @param servname ポート番号
 * @param Dflag 通信デバイスの指定フラグ
 * - NTCP: TCP/IP による接続
 *
 * @retval 成功 ネットワーク情報構造体
 * @retval 失敗 NULL 
*/
NET *setnet(char *hostname, char *sesrvname, uint32_t Dflag);

/**
 * @JP
 * @brief 接続先プロセスのネットワーク情報の解放
 *
 * freenet関数は、指定されたネットワーク情報構造体を解放する。
 *
 * @param nt ネットワーク情報構造体
 *
 * @retval なし 
*/
void freenet(NET *nt);

#endif
