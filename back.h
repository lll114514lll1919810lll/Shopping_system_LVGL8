#include "drivers.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"


//算总价（传入单价、数量，输出总价）
float AddPrice(float price , float count){
	return price*count;
}

//满减（传入总价、限额、减价，输出最终价）
float Minus(float before , float restriction , float MinusPrice){
	if(before >= restriction)
		return (before - MinusPrice);
	else
		return before;
}

//打折（同理）
float Discount(float before , float restriction , float discount){
	if(before >= restriction)
		return discount * before;
	else
		return before;
}

//用结构体储存交易记录(假设有5种商品，3种优惠）
struct Record
{
	float count[5]; //每种商品购买数量
	int IsFloat[5]; //1为按斤卖，0为按个卖
	float price[5]; //每种商品折前总价
	float before; //折前总价
	float after; //折后总价
	int discount[4]; //优惠类型，3为无优惠
};

/*一个函数，让打折界面至多一个开关打开？*/

/*其他函数...*/