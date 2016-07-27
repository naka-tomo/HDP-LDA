#pragma once
/*******************************************************************

HierarchicalDP.h		developed by naka_t	2010.1.12

	階層ディリクレ過程

  Copyright (C) 2011  naka_t <naka_t@apple.ee.uec.ac.jp>
 *******************************************************************/
#include "HDPTable.h"
#include "HDPDish.h"

#define BUFFER_SIZE				10000	// 計算用のバッファサイズ（最初に一括で確保）
#define HDP_CONCPARA_PRIOR_A	1.0		// ガンマ事前分布のパラメタ	
#define HDP_CONCPARA_PRIOR_B	1.0		// ガンマ事前分布のパラメタ

double RandF();
unsigned int RandD(unsigned int max );

class CHierarchicalDP
{
public:
	CHierarchicalDP(void);
	~CHierarchicalDP(void);

	
	void SetData( double *data[] , int dataNum , int dataDim );		// データのセット
	void Update();													// GibbsSamplingによりテーブルと料理をサンプリング
	void SaveResult( const char *dir );								// 結果の保存
	std::vector<std::vector<double>> GetPz_dk();					// 各データの尤度を取得
	std::vector<int> GetClassificationResult_d();					// 分類結果を取得

protected:
	/* 
	接尾辞
	 d : ドキュメントのインデックス（物体数）
	 t : テーブルのインデックス
	 k : 料理のインデックス
	 w : 単語のインデックス
	 l : 文書の長さ（単語数）のインデックス
	*/
	struct Document{						// チェーン店（文書）構造体
		int length;							// 人の数（文書の長さ）
		int *wordID_l;						// 各人（単語）のID
		int *table_l;						// 各人（単語）が座っているテーブル
		std::vector<CHDPTable> tables_t;	// 店舗に置かれているテーブル
	}*m_documents_d;

	std::vector<CHDPDish> m_dishes_k;	// 料理
	int m_dataDim;
	int m_dataNum;
	int *m_dataIndices_d;				// データのインデックス（データの順番を入れ替えるために必要）
	double *m_P;						// 計算用バッファ

	void Release();						// メモリ解放
	
	void UpdateTable( Document &d , int l );						// テーブルの更新
	int SamplingTable( std::vector<CHDPTable> &tables , int w );	// テーブルのサンプリング
	void DeleteEmptyTables( Document &d );							// 誰も座っていないテーブルを削除
	void DeleteTable( Document &d , int t );						// テーブルtを削除

	void UpdateDish( CHDPTable &table );							// 料理の更新
	int SamplingDish( std::vector<int> &w );						// 料理をサンプリング
	void DeleteEmptyDish();											// 誰も食べていない料理を削除
	void DeleteDish(int k);											// 料理kを削除
	void PutDishOnTable( CHDPTable &table , int k );				// テーブルに料理kを置く
	void RemoveDishFromTable( CHDPTable &table , int k );			// テーブルから料理を削除

	double m_lambda;							// テーブルを生成する確率の係数λ
	double m_gamma;								// 料理を生成する確率の係数γ
	double SamplingLambda( double oldLambda );	// λのサンプリング
	double SamplingGamma( double oldGamma );	// γのサンプリング
};
