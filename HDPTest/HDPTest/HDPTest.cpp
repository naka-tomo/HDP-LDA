/*******************************************************************

HDPTest.cpp		developed by naka_t	2011.02.10

	HDPクラスの使用例

  Copyright (C) 2011  naka_t <naka_t@apple.ee.uec.ac.jp>
 *******************************************************************/
#include "stdafx.h"
#include "HierarchicalDP.h"
#include "utility.h"


int _tmain(int argc, _TCHAR* argv[])
{
	CHierarchicalDP hdp;	// HDPクラス
	int num, dim;			// データの数と次元

	// データ読み込み
	double **data = LoadMatrix<double>( dim , num , "sample.txt" );

	// データをクラスに渡す
	hdp.SetData( data , num , dim );

	// gibbs sampling でパラメタを更新
	for(int i=0 ; i<100 ; i++ )
	{
		hdp.Update();
	}

	// 結果を保存
	hdp.SaveResult( "result" );

	Free( data );

	return 0;
}

