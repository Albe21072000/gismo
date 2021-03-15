#pragma once
/** @file gsNURBSinfo.hpp

	@brief RM shell's NURBS information.

	This file is not part of the G+Smo library.

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	Author(s): HS. Wang

	Date:   2020-12-30
*/

#include "gsNURBSinfo.h"

namespace gismo
{
	/* ����1 ������multibasis.basis(0)����Ľ��������������ʽ���룬
	* �÷����Ĳ�����ȫ���ܱ߽���������txt�ļ������������
	* �÷�����Ҫ������ַ������͵����ݽ��д���
	* ����Ҫ��ȡ��maxSpan=0.25)���еġ�0.25�����������Ѳ����ģ�
	* ��Ϊ�ú������ַ������ݵĴ�����м��ߵĲο���ֵ��
	* Ҳ�Ǳ��˻��Ѻܴ�����Ƴ����ķ�����
	* �������˷���2���ֶ�������������Էǳ��õķ���
	* ��ϣ���Է���1���б������������˼�������õ��ϵ�ʱ��
	* ����˧ 2021-01-30 */

	// ��ȡ�ڵ�������Ϣ�ĺ���ģ��
	template < class T>
	void gsNURBSinfo<T>::getKnotVectorInfo()
	{
		std::ifstream info_in("NURBSinfo.txt");

		std::string line;
		std::stringstream ss_buff;
		std::string temps;

		index_t knot_dir = 0;

		while (!info_in.eof())
		{
			getline(info_in, line);

			if (line.find("Direction") != std::string::npos)
			{
				ss_buff.str("");
				ss_buff.str(line);

				ss_buff >> temps;	// Direction
				ss_buff >> temps;	//	0:
				info_iga.knot_direction = knot_dir;

				// �ڵ�����
				ss_buff >> temps;	// [
				ss_buff >> temps;	// 0
				std::string judge_symbol("]");
				while (temps != judge_symbol)
				{
					tran_number = trans_str_to_any(temps);
					info_iga.knot_vector.push_back(tran_number);	//	�ڵ�����
					ss_buff >> temps;	// 0 0 1 1 1 ]
				}
				// =============================================
				ss_buff >> temps; // (deg=2,
				std::string dig_str;
				for (auto ca : temps)
				{
					if (isdigit(ca))
					{
						dig_str += ca;
					}
				}
				info_iga.knot_degree = trans_str_to_any(dig_str);
				// ============================================
				ss_buff >> temps; // size=9,
				std::string siz_str;
				for (auto cb : temps)
				{
					if (isdigit(cb))
					{
						siz_str += cb;
					}
				}
				info_iga.knot_size = trans_str_to_any(siz_str);
				// ============================================
				ss_buff >> temps; // minSpan=0.25
				std::string mins_str;
				//char m_dot('.');	
				for (char cc : temps)
				{
					if (isdigit(cc) || cc == '.')
					{
						mins_str += cc;
					}
				}
				info_iga.knot_minSpan = trans_str_to_any(mins_str);
				// ===========================================
				ss_buff >> temps; // maxSpan=0.25)
				std::string maxs_str;
				for (char cd : temps)
				{
					if (isdigit(cd) || cd == '.')
					{
						maxs_str += cd;
					}
				}
				info_iga.knot_maxSpan = trans_str_to_any(maxs_str);

				ss_buff.clear();

				igainfo.push_back(info_iga);
				info_iga.initialize();
				//igainfo[knot_dir]=(info_iga);
				++knot_dir;
			}

			else if (!line.empty())
			{ // TensorBSplineBasis: dim=2, size=36.
				ss_buff.str("");
				ss_buff.str(line);

				ss_buff >> basis_type;	// TensorBSplineBasis:
				// ===========================================
				ss_buff >> temps; // dim=2,
				// ��ȡ��ֵ
				std::string dim_str;
				for (auto ce : temps)
				{
					if (isdigit(ce)) // ���������
					{
						dim_str += ce;
					}
				}
				nurbs_dim = trans_str_to_any(dim_str);
				// ===========================================
				ss_buff >> temps; // size=4.
				std::string size_str;
				for (auto cf : temps)
				{
					if (isdigit(cf))
					{
						size_str += cf;
					}
				}
				nurbs_node = trans_str_to_any(size_str);

				ss_buff.clear();
			}

		}

		gsInfo << basis_type << " ";
		gsInfo << "dim=" << nurbs_dim << ", ";
		gsInfo << "size=" << nurbs_node << ".\n";

		for (index_t j = 0; j < igainfo.size(); ++j)
		{
			gsInfo << "  Direction " << j << ": [ ";
			for (index_t k = 0; k < igainfo[j].knot_vector.size(); ++k)
			{
				gsInfo << igainfo[j].knot_vector[k] << " ";
			}
			gsInfo << "] (";
			gsInfo << "deg=" << igainfo[j].knot_degree
				<< ", size=" << igainfo[j].knot_size
				<< ", minSpan=" << igainfo[j].knot_minSpan
				<< ", maxSpan=" << igainfo[j].knot_maxSpan;
			gsInfo << ")\n";
		}

	}

	// �ַ���ת��Ϊ���ֵĺ���ģ��
	template<typename T>
	T gsNURBSinfo<T>::trans_str_to_any(std::string s_str)
	{
		T res;
		std::stringstream Repeater;
		Repeater << s_str;
		Repeater >> res;
		return res;
	}

	// >>> --------------------------------------------------------------------
	// ���K�ļ�
	// ����1
	template < class T>
	void gsNURBSinfo<T>::OutputKfile(const gsGeometry<T>& Geo, const std::string& name)
	{
		gsMapData<T> map_data;
		map_data.flags = NEED_VALUE | NEED_JACOBIAN | NEED_GRAD_TRANSFORM;
		gsGenericGeometryEvaluator<T, 2, 1> geoEval(Geo, map_data.flags);
		gsMatrix<real_t> globalCp;
		globalCp = geoEval.geometry().coefs();

		ofstream outK(name.c_str());
		outK << "*KEYWORD\n*NODE\n";
		//ѭ��ÿһ��Ƭ ����ڵ�/���Ƶ���Ϣ
		for (index_t i = 0; i < nurbs_node; ++i)
		{
			outK << setw(8) << i + 1;
			outK << setw(16) << globalCp(i, 0);
			outK << setw(16) << globalCp(i, 1);
			outK << setw(16) << globalCp(i, 2);
			outK << "\n";
		}

		outK << "*PART\n";
		outK << "*ELEMENT_SHELL\n";
		unsigned int jj = 1;
		for (unsigned int i = 0; i < multi_patch.nPatches(); ++i)
		{
			index_t ele_row = igainfo[0].knot_size - 1;
			index_t ele_col = igainfo[1].knot_size - 1;

			for (unsigned int j = 0; j < ele_col; ++j)
			{
				for (unsigned int k = 0; k < ele_row; ++k)
				{
					index_t st = k + (ele_row + 1) * j;
					outK << setw(8) << jj; // ��������Ԫ���
					outK << setw(8) << 1;
					outK << setw(8) << (st + 1) << setw(8) << (st + 2); // ��������ڵ���
					outK << setw(8) << (st + 2 + ele_row)
						<< setw(8) << (st + 1 + ele_row) << endl;
					jj++;
				}
			}
		}
		outK << "*END";
		outK.close();
		gsInfo << "Export HyperMesh k files: " << name.c_str() << "\n";

	}
	// ����2
	template < class T>
	void gsNURBSinfo<T>::ExportKfile(const std::string& name)
	{
		ofstream outK(name.c_str());
		//ѭ��ÿһ��Ƭ ����ڵ�/���Ƶ���Ϣ
		outK << "*KEYWORD\n*NODE\n";
		index_t tempi = 1;
		for (index_t i = 0; i < nurbs_sumpatch; ++i)
		{
			for (index_t j = 0; j < nurbsdata[i].patch_sumcp; ++j)
			{
				outK << setw(8) << nurbsdata[i].cp_no[j] + 1;
				outK << setw(16) << nurbsdata[i].phys_coor(j, 0);
				outK << setw(16) << nurbsdata[i].phys_coor(j, 1);
				outK << setw(16) << nurbsdata[i].phys_coor(j, 2);
				outK << "\n";
				++tempi;
			}
		}
		// �����Ԫ��Ϣ
		//outK << "*PART\n";
		outK << "*ELEMENT_SHELL\n";
		unsigned int jj = 1;
		for (unsigned int i = 0; i < nurbs_sumpatch; ++i)
		{
			index_t cp_row = nurbsdata[i].knot_vector[0].knotvec_size;
			index_t cp_col = nurbsdata[i].knot_vector[1].knotvec_size;
			index_t ele_row = cp_row - 1;
			index_t ele_col = cp_col - 1;
			vector<int> seq = nurbsdata[i].cp_no;// ���Ƶ���
			for (unsigned int j = 0; j < ele_row; ++j)
			{
				for (unsigned int k = 0; k < ele_col; ++k)
				{
					index_t st = k + cp_col * j;
					outK << setw(8) << jj; // ��������Ԫ���
					outK << setw(8) << i + 1;
					// ��������ڵ���
					outK << setw(8) << (seq[st] + 1)
						<< setw(8) << (seq[st + 1] + 1)
						<< setw(8) << (seq[st + cp_col + 1] + 1)
						<< setw(8) << (seq[st + cp_col] + 1) << endl;
					++jj;
				}
			}
		}
		//outK << "*END";
		outK.close();
		gsInfo << "Export HyperMesh k files: " << name.c_str() << "\n";
	}
	// -------------------------------------------------------------------- <<<
}




