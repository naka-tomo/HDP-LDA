#pragma once
/*******************************************************************

HDPDish.h		developed by naka_t	2011.01.12

	HDPのテーブルに置かれる料理の情報を保持するクラス

  Copyright (C) 2011  naka_t <naka_t@apple.ee.uec.ac.jp>
*******************************************************************/
#include <vector>

#define HDP_ALPHA	0.1

class CHDPDish
{
public:
	CHDPDish(int wordNum );
	~CHDPDish(void);

	void AddData( int w );								// この料理を食べている人を追加
	void AddData( std::vector<int> &w );				// この料理を食べている複数の人を追加
	void DeleteData( int w );							// この料理を食べている人を除外
	void DeleteData( std::vector<int> &w );				// この料理を食べている複数の人を除外

	void UpPopularity();								// この料理の人気を上げる（食べられているテーブル数を追加）
	void DownPopularity();								// この料理の人気を下げる（食べられているテーブル数を減少）
	int GetPopularity();								// この料理の人気を取得（食べらているテーブル数を取得）

	double CalcLogLikilihood( int w );					// 人(w：単語)がこの料理（トピック）を好む尤度	
	double CalcLogLikilihood( std::vector<int> w );		// 複数の人(w：単語)がこの料理（トピック）を好む尤度	

protected:
	std::vector<int> m_N_w;		// この料理を食べている人（単語）の数の内訳
	int m_N;					// この料理を食べている人（単語）の合計
	int m_numTables;			// この料理が食べられているテーブルの数
	int m_wordNum;
};
