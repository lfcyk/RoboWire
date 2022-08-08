// ＣＧ課題配布コード
// シェーディング基礎プログラム　M.Fujio(2016.10.27)

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

///*-- データの定義	------------------------------------*/
//
// 画面サイズの定義
#define SCREEN_SIZE_X	640								// 初期画面サイズ X
#define SCREEN_SIZE_Y	640								// 初期画面サイズ Y
#define INIT_POS_X		200								// 初期画面位置 X
#define INIT_POS_Y		200								// 初期画面位置 Y
// グローバル変数の定義
int gnScreenSizeX = SCREEN_SIZE_X;						// 画面の横サイズ
int gnScreenSizeY = SCREEN_SIZE_Y;						// 画面の縦サイズ
// ワイヤモデルと部品の最大数の設定
#define MAX_LINES	50									// モデルのLINE線の最大数
#define BODY_NUM	10									// ロボットのモデル構成の最大数
#define MAX_POLYS	50

///*--- ワイヤモデルデータ -----------------------------*/
// ワイヤー１本データ
struct WIRE {
	double dPos[2][4];									// 始点終点座標 [Start/End][X/Y/Z]
};
// ワイヤーモデルデータ
struct WIRE_MODEL {
	int nWireNum;
	struct WIRE Wire[MAX_LINES];
};
WIRE_MODEL WireModel;

struct STL {
	double dPos[3][4];	
	double dN[4];
};

struct STL_MODEL {
	int nPolyNum;
	struct STL Stl[MAX_POLYS];
};
STL_MODEL StlModel;

///*--- モデリング変換パラメータ -----------------------*/
struct MODELING {
	double dScale[3];									// スケーリング
	double dRotate[3];									// 回転
	double dTrans[3];									// 平行移動
	double dTransMatrix[4][4];							// 合成行列
};

//*--- ロボットモデルデータ ----------------------------*/
struct MODELING_ROBO {
	int nTransNum;
	MODELING Modeling[BODY_NUM];
	WIRE_MODEL RoboWireModel[BODY_NUM];
	STL_MODEL RoboPolyModel[BODY_NUM];
};
MODELING_ROBO ModelRobo;

//*-- 投影情報 -----------------------------------------*/
// 視点位置
double dViewDepth = 500;								// 中心までの距離
double dViewEle = 0;									// XY平面からの角度
double dViewAzim = 90.;									// Z軸周りの角度
double dViewPos[3] = { 0, 100, 0 };						// 視点座標
// 光源位置(Light0)
double dLightDepth = 50;								// 中心までの距離
double dLightEle = 50;									// XY平面からの角度
double dLightAzim = 90;									// Z軸周りの角度(X軸から右+)
GLfloat dLightPos[4] = { 0, 0, 0, 1 };					// 光源位置の設定
GLfloat dLightCol[4] = { 1, 1, 1, 1 };					// 光源の色
GLfloat dAmbient[4] = { 0.5, 0.5, 0.5, 1 };				// 環境光
// ワイヤーの色
GLfloat mat[4] = { 0.5, 0.7, 1.0, 1.0 };			// 色データの作成(RGBA)

/*-- インタラクティブ　ソースコード -------------------*/
double dMouseMove[2] = { 0.0, 0.0 };					// マウスの位置
int    nMouseButton = 0;								// マウスのボタンの状態 0:押されていない 1:左 2:右

// 特殊キー入力処理
void SpecialKeyIn(int key, int x, int y)
{
	// 視点パラメータの更新
	switch (key) {
	case GLUT_KEY_RIGHT:								// →キー入力
		dViewAzim -= 5.0;								// 視点左移動
		if (dViewAzim < 0)
			dViewAzim = 360;
		printf("dAzim = %8.3lf\n", dViewAzim);
		break;
	case GLUT_KEY_LEFT:									// ←キー入力
		dViewAzim += 5.0;								// 視点上移動
		if (dViewAzim > 360)
			dViewAzim = 0;
		printf("dAzim = %8.3lf\n", dViewAzim);
		break;
	case GLUT_KEY_DOWN:									// ↓キー入力
		dViewEle += 5.0;								// 視点上移動
		if (dViewEle >= 90)
			dViewEle = 89;
		printf("dEle = %8.3lf\n", dViewEle);
		break;
	case GLUT_KEY_UP:									// ↑キー入力
		dViewEle -= 5.0;								// 視点下移動
		if (dViewEle < 0)
			dViewEle = 0;
		printf("dEle = %8.3lf\n", dViewEle);
		break;
	default:
		break;
	}
	// 視点の再計算
	double dE = (90 - dViewEle) * M_PI / 180.;
	double dA = dViewAzim * M_PI / 180.;
	dViewPos[0] = dViewDepth * sin(dE) * cos(dA);
	dViewPos[1] = dViewDepth * sin(dE) * sin(dA);
	dViewPos[2] = dViewDepth * cos(dE);
	// 再描画
	glutPostRedisplay();								// 再描画
}

// 一般キー入力処理
void NormalKeyIn(unsigned char key, int x, int y)
{
	int nFlg = 1;										// 視点変更(1)or光源変更(!1)
	switch (key) {
	case 27:											// ESCキー入力
		exit(0);										// プログラム終了
		break;
	case 'i':											// 'i'キー入力
		dViewDepth -= 5.0;								// ZOOM IN
		if (dViewDepth <= 10)
			dViewDepth = 10;
		printf("dDepth = %8.3lf\n", dViewDepth);
		break;
	case 'o':											// 'o'キー入力
		dViewDepth += 5.0;								// ZOOM OUT
		if (dViewDepth >= 1000)
			dViewDepth = 1000;
		printf("dDepth = %8.3lf\n", dViewDepth);
		break;
	case 'l':											// 'l'キー入力
		nFlg = 0;										// 光源左移動
		dLightAzim -= 5.0;
		if (dLightAzim < 0)
			dLightAzim = 360;
		printf("dLightAzim = %8.3lf\n", dLightAzim);
		break;
	case 'r':											// 'r'キー入力
		nFlg = 0;										// 光源右移動
		dLightAzim += 5.0;
		if (dLightAzim > 360)
			dLightAzim = 0;
		printf("dLightAzim = %8.3lf\n", dLightAzim);
		break;
	case 'u':											// 'u'キー入力
		nFlg = 0;										// 光源上移動
		dLightEle += 5.0;
		if (dLightEle >= 90)
			dLightEle = 90;
		printf("dLightEle = %8.3lf\n", dLightEle);
		break;
	case 'd':											// 'd'キー入力
		nFlg = 0;										// 光源下移動
		dLightEle -= 5.0;
		if (dLightEle <= 0)
			dLightEle = 0;
		printf("dLightEle = %8.3lf\n", dLightEle);
		break;
	default:
		break;
	}
	double dE, dA;
	if (nFlg) {
		// 視点の再計算
		dE = (90 - dViewEle) * M_PI / 180.;
		dA = dViewAzim * M_PI / 180.;
		dViewPos[0] = dViewDepth * sin(dE) * cos(dA);
		dViewPos[1] = dViewDepth * sin(dE) * sin(dA);
		dViewPos[2] = dViewDepth * cos(dE);
	}
	else {
		// 光源位置の再計算
		dE = (90 - dLightEle) * M_PI / 180.;
		dA = dLightAzim * M_PI / 180.;
		dLightPos[0] = GLfloat(dLightDepth * sin(dE) * cos(dA));
		dLightPos[1] = GLfloat(dLightDepth * sin(dE) * sin(dA));
		dLightPos[2] = GLfloat(dLightDepth * cos(dE));
	}
	// 再描画
	glutPostRedisplay();								// 再描画
}

// マウスボタンクリック時のボタン状態の設定
void MouseButtonOn(int nButton, int nState, int x, int y)
{
	// マウスボタンが押されかを判定
	if (nState == GLUT_DOWN) {
		switch (nButton) {
		case GLUT_LEFT_BUTTON:							// マウス左ボタンクリック時
			nMouseButton = 1;							// マウス状態設定
			break;
		case GLUT_RIGHT_BUTTON:							// マウス右ボタンクリック時
			nMouseButton = 2;							// マウス状態設定
			break;
		default:
			nMouseButton = 0;							// マウス状態フリー定
			break;
		}
		// マウス位置を保存
		dMouseMove[0] = (double)x;						// マウス位置 x の保存
		dMouseMove[1] = (double)y;						// マウス位置 y の保存
	}
	else
		nMouseButton = 0;								// ボタン状態フリー
}

// マウスドラッグ時の処理
void MouseDrag(int x, int y)
{
	// 移動量の計算
	double dx, dy;
	dx = (double)x - dMouseMove[0];
	dy = (double)y - dMouseMove[1];
	if (nMouseButton == 1) {								// マウスの左ボタンドラッグ
		dViewAzim -= dx / 50.;							// RIGHT/LEFT
		dViewEle += dy / 50.;							// UP/DOWN
	}
	else if (nMouseButton == 2) {						// マウスの右ボタンドラッグ
		dViewDepth -= dy / 50.;							// ZOOM IN/OUT
	}
	// パラメータのチェック
	if (dViewDepth <= 10)			dViewDepth = 10;
	else if (dViewDepth >= 1000)	dViewDepth = 1000;
	if (dViewAzim < 0)				dViewAzim = 360;
	else if (dViewAzim >= 360)		dViewAzim = 0;
	if (dViewEle <= 0)				dViewEle = 0;
	else if (dViewEle >= 90)		dViewEle = 89;
	// 視点の再計算
	double dE = (90 - dViewEle) * M_PI / 180.;
	double dA = dViewAzim * M_PI / 180.;
	dViewPos[0] = dViewDepth * sin(dE) * cos(dA);
	dViewPos[1] = dViewDepth * sin(dE) * sin(dA);
	dViewPos[2] = dViewDepth * cos(dE);
	// 再描画
	glutPostRedisplay();								// 再描画
}

//*-- ソースコード -------------------------------------*/
//　画面サイズの更新（w:画面幅　h:画面高さ）
void Resize(int w, int h)
{
	double aspect = (double)w / (double)h;				// アスペクト比計算
	glViewport(0, 0, w, h);								// ビューポート設定
	glMatrixMode(GL_PROJECTION);						// 投影モード切替
	glLoadIdentity();									// 投影行列の初期化
	gluPerspective(30.0, aspect, 1.0, 5000.0);			// 透視投影の設定
	glMatrixMode(GL_MODELVIEW);
}

// シーンの描画
void Rendering(void)
{
	// 画面のクリア(Z-Buffer,Stencil-Buffer)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// カメラ設定			
	glMatrixMode(GL_MODELVIEW);							// 投影モード切替
	glLoadIdentity();									// 投影行列の初期化
	gluLookAt(dViewPos[0], dViewPos[1], dViewPos[2], 0, 0, 0, 0, 0, 1);
	// 光源の設定
	glLightfv(GL_LIGHT0, GL_POSITION, dLightPos);		// 光源位置の設定
	glLightfv(GL_LIGHT0, GL_DIFFUSE, dLightCol);		// 光源の色(ライトの色)設定
	glLightfv(GL_LIGHT0, GL_AMBIENT, dAmbient);			// 環境光の設定
	// モデルの色情報の設定
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat);			// 表面色の設定
	

	// XYZ軸の描画
	//glBegin(GL_LINES);
	//glVertex3d(0, 0, 0); glVertex3d(100, 0, 0);
	//glVertex3d(0, 0, 0); glVertex3d(0, 100, 0);
	//glVertex3d(0, 0, 0); glVertex3d(0, 0, 100);
	//glEnd();

	// RoboWireの表示
	//-- ソースを記入(開始)
	//glBegin(GL_LINES);
	//for(int i=0;i<ModelRobo.nTransNum;i++)
	//	for (int j = 0; j < WireModel.nWireNum; j++) {
	//		glVertex3d(ModelRobo.RoboWireModel[i].Wire[j].dPos[0][0], ModelRobo.RoboWireModel[i].Wire[j].dPos[0][1], ModelRobo.RoboWireModel[i].Wire[j].dPos[0][2]);
	//		glVertex3d(ModelRobo.RoboWireModel[i].Wire[j].dPos[1][0], ModelRobo.RoboWireModel[i].Wire[j].dPos[1][1], ModelRobo.RoboWireModel[i].Wire[j].dPos[1][2]);
	//	}
	//glEnd();

	glShadeModel(GL_FLAT);
	for (int k = 0; k < ModelRobo.nTransNum; k++) {
		for (int j = 0; j < StlModel.nPolyNum; j++) {
			glNormal3d(ModelRobo.RoboPolyModel[k].Stl[j].dN[0], ModelRobo.RoboPolyModel[k].Stl[j].dN[1], ModelRobo.RoboPolyModel[k].Stl[j].dN[2]);
			glBegin(GL_TRIANGLES);
			for (int i = 0; i < 4; i++) {
				glVertex3d(ModelRobo.RoboPolyModel[k].Stl[j].dPos[i][0], ModelRobo.RoboPolyModel[k].Stl[j].dPos[i][1], ModelRobo.RoboPolyModel[k].Stl[j].dPos[i][2]);
			}
			glEnd();
		}
	}
	
	//-- ソースを記入(終了)

	// バッファの更新
	glutSwapBuffers();
}

// データの読み込み
//　　正常終了 1 / エラー 0
int ReadData(void) {

	char strBuf[256];
	int i;

	// ワイヤーモデルのオープン
	FILE* pf;
	int nflag=0;
	pf = fopen("WireCylinder.txt", "r");
	if (pf == NULL) return 0;
	// ファイル読み込み
	if( (nflag = fscanf(pf, "%s", strBuf)) < 0) {
		printf("File Read Error 1\n");
		return 0;
	}
	if ((nflag = fscanf(pf, "%d", &WireModel.nWireNum)) < 0) {
		printf("File Read Error 2\n");
		return 0;
	}
	for (i = 0; i < WireModel.nWireNum; i++) {
		nflag = fscanf(pf, "%lf,%lf,%lf %lf,%lf,%lf",
			&WireModel.Wire[i].dPos[0][0], &WireModel.Wire[i].dPos[0][1], &WireModel.Wire[i].dPos[0][2],
			&WireModel.Wire[i].dPos[1][0], &WireModel.Wire[i].dPos[1][1], &WireModel.Wire[i].dPos[1][2]);
		if (nflag < 0) {
			printf("File Read Error 3\n");
			return 0;
		}
		WireModel.Wire[i].dPos[0][3] = WireModel.Wire[i].dPos[1][3] = 1.0;
	}
	fclose(pf);

	// モデリング変換パラメータ
	pf = fopen("modeling.txt", "r");
	if (pf == NULL) return 0;
	// ファイル読み込み
	//-- ソースを記入(開始)
	if ((nflag = fscanf(pf, "%s", strBuf)) < 0) {
		printf("File Read Error 1\n");
		return 0;
	}
	if ((nflag = fscanf(pf, "%d", &ModelRobo.nTransNum)) < 0) {
		printf("File Read Error 2\n");
		return 0;
	}
	for (i = 0; i < ModelRobo.nTransNum; i++) {
		if ((nflag = fscanf(pf, "%s", strBuf)) < 0) {
			printf("File Read Error 1\n");
			return 0;
		}
		nflag = fscanf(pf, "%lf,%lf,%lf", &ModelRobo.Modeling[i].dScale[0], &ModelRobo.Modeling[i].dScale[1], &ModelRobo.Modeling[i].dScale[2]);
		nflag = fscanf(pf, "%lf,%lf,%lf", &ModelRobo.Modeling[i].dRotate[0], &ModelRobo.Modeling[i].dRotate[1], &ModelRobo.Modeling[i].dRotate[2]);
		nflag = fscanf(pf, "%lf,%lf,%lf", &ModelRobo.Modeling[i].dTrans[0], &ModelRobo.Modeling[i].dTrans[1], &ModelRobo.Modeling[i].dTrans[2]);
		if (nflag < 0) {
			printf("File Read Error 3\n");
			return 0;
		}
	}

	//-- ソースを記入(終了)
	fclose(pf);

	return(1);
}

int ReadPolygon(void) {
	char strBuf[256];
	int i,j;

	// ワイヤーモデルのオープン
	FILE* pf;
	int nflag = 0;
	pf = fopen("CylinderFlat.txt", "r");
	if (pf == NULL) return 0;
	// ファイル読み込み
	if ((nflag = fscanf(pf, "%d", &StlModel.nPolyNum)) < 0) {
		printf("File Read Error 2\n");
		return 0;
	}
	for (i = 0; i < StlModel.nPolyNum; i++) {
		nflag = fscanf(pf, "%lf,%lf,%lf",
			&StlModel.Stl[i].dN[0], &StlModel.Stl[i].dN[1], &StlModel.Stl[i].dN[2]);
		if (nflag < 0) {
			printf("File Read Error 3\n");
			return 0;
		}
		for (j = 0; j < 3; j++) {
			nflag = fscanf(pf, "%lf,%lf,%lff",
				&StlModel.Stl[i].dPos[j][0], &StlModel.Stl[i].dPos[j][1], &StlModel.Stl[i].dPos[j][2]);
			if (nflag < 0) {
				printf("File Read Error 3\n");
				return 0;
			}
		}
		StlModel.Stl[i].dN[3] = StlModel.Stl[i].dPos[0][3] = StlModel.Stl[i].dPos[1][3]= StlModel.Stl[i].dPos[2][3] =1.0;
	}
	fclose(pf);

	return(1);
}


// 行列を単位行列にする
void CalMatrixUnit(double M[4][4])
{
	// 全要素をZEROクリアー
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			M[i][j] = 0.0;
	// 対角行列の設定
	M[0][0] = M[1][1] = M[2][2] = M[3][3] = 1.0;
}

// 行列計算関数 T = A * B を計算する
void CalMatrix(double A[4][4], double B[4][4], double T[4][4])
{
	//-- ソースを記入(開始)

	// 全要素をZEROクリアー
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			T[i][j] = 0.0;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			for (int k = 0; k < 4; ++k) {
				T[i][j] +=  A[i][k] * B[k][j];
			}
		}
	}
	//-- ソースを記入(終了)
}
// 座標計算関数 T = A * B を計算する
void CalPos(double A[4][4], double B[4], double T[4]) {
	for (int i = 0; i < 4; i++) {
		T[i] = 0.0;
		for (int j = 0; j < 4; j++)
			T[i] += A[i][j] * B[j];
	}
}
// 座標計算

void CalNormalVec(void) {
	/*for (int k = 0; k < ModelRobo.nTransNum; k++) {
		for (int j = 0; j < StlModel.nPolyNum; j++) {
			double U[3] = { 0,0,0 }, V[3] = { 0,0,0 };
			for (int i = 0; i < 3; i++) {
				U[i] = ModelRobo.RoboPolyModel[k].Stl[j].dPos[1][i] - ModelRobo.RoboPolyModel[k].Stl[j].dPos[0][i];
				V[i] = ModelRobo.RoboPolyModel[k].Stl[j].dPos[2][i] - ModelRobo.RoboPolyModel[k].Stl[j].dPos[0][i];
			}
			ModelRobo.RoboPolyModel[k].Stl[j].dN[0] = (U[1] * V[2]) - (U[2] * V[1]);
			ModelRobo.RoboPolyModel[k].Stl[j].dN[1] = (U[2] * V[0]) - (U[0] * V[2]);
			ModelRobo.RoboPolyModel[k].Stl[j].dN[2] = (U[0] * V[1]) - (U[1] * V[0]);
		}
	}*/
	for (int i = 0; i < ModelRobo.nTransNum; i++) {
		double Temp[3][4][4];
		CalMatrixUnit(Temp[0]);
		//Temp[0] = Tγ
		Temp[0][0][0] = Temp[0][1][1] = cos(ModelRobo.Modeling[i].dRotate[2] * M_PI / 180.);
		Temp[0][0][1] = -sin(ModelRobo.Modeling[i].dRotate[2] * M_PI / 180.);
		Temp[0][1][0] = -Temp[0][0][1];
		CalMatrixUnit(Temp[1]);
		//Temp[1] = Tβ
		Temp[1][0][0] = Temp[1][2][2] = cos(ModelRobo.Modeling[i].dRotate[1] * M_PI / 180.);
		Temp[1][0][2] = sin(ModelRobo.Modeling[i].dRotate[1] * M_PI / 180.);
		Temp[1][2][0] = -Temp[1][0][2];
		CalMatrixUnit(Temp[1]);

		CalMatrix(Temp[0], Temp[1], Temp[2]);

		//Temp[1]=Tα
		Temp[1][2][2] = Temp[1][1][1] = cos(ModelRobo.Modeling[i].dRotate[0] * M_PI / 180.);
		Temp[1][2][1] = sin(ModelRobo.Modeling[i].dRotate[0] * M_PI / 180.);
		Temp[1][1][2] = -Temp[1][2][1];

		CalMatrix(Temp[2], Temp[1], Temp[0]);
		for (int j = 0; j < StlModel.nPolyNum; j++) {
			CalPos(Temp[0], StlModel.Stl[j].dN, ModelRobo.RoboPolyModel[i].Stl[j].dN);
		}
	}


}
void ModelingTransform(void)
{
	double Temp[3][4][4];								// テンポラリな行列
	// ６この部位について座標変換を行う
	for (int i = 0; i < ModelRobo.nTransNum; i++) {
		// LINE数のコピー
		ModelRobo.RoboPolyModel[i].nPolyNum = StlModel.nPolyNum;
		// 平行移動行列の設定
		CalMatrixUnit(Temp[0]);
		Temp[0][0][3] = ModelRobo.Modeling[i].dTrans[0];
		Temp[0][1][3] = ModelRobo.Modeling[i].dTrans[1];
		Temp[0][2][3] = ModelRobo.Modeling[i].dTrans[2];
		// Z軸周りの回転
		CalMatrixUnit(Temp[1]);
		Temp[1][0][0] = Temp[1][1][1] = cos(ModelRobo.Modeling[i].dRotate[2] * M_PI / 180.);
		Temp[1][0][1] = -sin(ModelRobo.Modeling[i].dRotate[2] * M_PI / 180.);
		Temp[1][1][0] = -Temp[1][0][1];
		// T = Tt * Trz
		CalMatrixUnit(Temp[2]);
		CalMatrix(Temp[0], Temp[1], Temp[2]);
		//-- ソースを記入(開始)
		CalMatrixUnit(Temp[0]);
		CalMatrixUnit(Temp[1]);

		//Temp[2] = Tt*Tγ
		Temp[1][0][0] = Temp[1][2][2] = cos(ModelRobo.Modeling[i].dRotate[1] * M_PI / 180.);
		Temp[1][0][2] = sin(ModelRobo.Modeling[i].dRotate[1] * M_PI / 180.);
		Temp[1][2][0] = -Temp[1][0][2];
		//Temp[1] = Tβ
		CalMatrix(Temp[2], Temp[1], Temp[0]); 
		
		//Temp[0] = Tt * Tγ*Tβ
		CalMatrixUnit(Temp[2]);
		CalMatrixUnit(Temp[1]);

		//Temp[1]=Tα
		Temp[1][2][2] = Temp[1][1][1] = cos(ModelRobo.Modeling[i].dRotate[0] * M_PI / 180.);
		Temp[1][2][1] = sin(ModelRobo.Modeling[i].dRotate[0] * M_PI / 180.);
		Temp[1][1][2] = -Temp[1][2][1];

		//Temp[2] = Tt * Tγ*Tβ*Tα
		CalMatrix(Temp[0], Temp[1], Temp[2]);

		CalMatrixUnit(Temp[0]);
		CalMatrixUnit(Temp[1]);

		Temp[1][0][0] = ModelRobo.Modeling[i].dScale[0];
		Temp[1][1][1] = ModelRobo.Modeling[i].dScale[1];
		Temp[1][2][2] = ModelRobo.Modeling[i].dScale[2];

		CalMatrix(Temp[2], Temp[1], ModelRobo.Modeling[i].dTransMatrix);

		for (int j = 0; j < StlModel.nPolyNum; j++) {
			CalPos(ModelRobo.Modeling[i].dTransMatrix, StlModel.Stl[j].dPos[0], ModelRobo.RoboPolyModel[i].Stl[j].dPos[0]);
			CalPos(ModelRobo.Modeling[i].dTransMatrix, StlModel.Stl[j].dPos[1], ModelRobo.RoboPolyModel[i].Stl[j].dPos[1]);
			CalPos(ModelRobo.Modeling[i].dTransMatrix, StlModel.Stl[j].dPos[2], ModelRobo.RoboPolyModel[i].Stl[j].dPos[2]);
			}

		//-- ソースを記入(終了)
	}
}

// OpenGL及びGlutの初期化
void InitOpenGL(void)
{
	// グラフィックの初期化
	int width = 480;									// 画面幅
	int height = 640;									// 画面高さ
	glutInitDisplayMode(GLUT_DEPTH | GLUT_RGBA | GLUT_DOUBLE);	// 色モード設定
	glutInitWindowSize(SCREEN_SIZE_X, SCREEN_SIZE_Y);	// 画面サイズの指定
	glutInitWindowPosition(INIT_POS_X, INIT_POS_Y);		// ウインドウの描画位置指定
	glutCreateWindow("3D Shading");						// 画面の生成	
	// 光源の設定(Light0を有効にする)
	glEnable(GL_LIGHTING);								// 光源の有効
	glEnable(GL_LIGHT0);								// 光源0の有効
	// 光源位置の初期計算
	double dE, dA;
	dE = (90 - dLightEle) * M_PI / 180.;
	dA = dLightAzim * M_PI / 180.;
	dLightPos[0] = GLfloat(dLightDepth * sin(dE) * cos(dA));
	dLightPos[1] = GLfloat(dLightDepth * sin(dE) * sin(dA));
	dLightPos[2] = GLfloat(dLightDepth * cos(dE));
	// 視点の初期計算
	dE = (90 - dViewEle) * M_PI / 180.;
	dA = dViewAzim * M_PI / 180.;
	dViewPos[0] = dViewDepth * sin(dE) * cos(dA);
	dViewPos[1] = dViewDepth * sin(dE) * sin(dA);
	dViewPos[2] = dViewDepth * cos(dE);
	
	// 画面の初期化
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// 背景色の設定(黒)
	// Z-Bufferの有効(無くすと前後処理ができなくなる)
	glEnable(GL_DEPTH_TEST);							// デプスバッファ(Z-Buffer)有効
}


// メイン処理
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);								// GLUTの初期化
	InitOpenGL();										// 投影の初期化
	if(!ReadData()) return(0);							// ワイヤーとモデリング変換データリード
	if (!ReadPolygon()) return(0);						// データリード
	ModelingTransform();								// 座標計算	
	CalNormalVec();
	glutDisplayFunc(Rendering);							// 投影関数の定義
	glutReshapeFunc(Resize);							// 画面サイズ変更関数(初期化でも実行)
	glutKeyboardFunc(NormalKeyIn);						// 一般キー入力処理関数の定義
	glutSpecialFunc(SpecialKeyIn);						// 特殊キー入力処理関数の定義
	glutMouseFunc(MouseButtonOn);						// マウスボタンクリック処理関数の定義
	glutMotionFunc(MouseDrag);							// マウスドラッグ処理関数の定義
	glutMainLoop();										// OpenGLループ
	return(1);
}
