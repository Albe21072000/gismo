#pragma once
/* =============================================================
*	ժҪ�����������ڵ���pd_xxx.hpp�ļ�����PD���ֵ���ڵ����ã��ն������ȡ�
*	���ߣ�����˧�� ������ ֣���� ��
*	���ڣ� 2021-01-25
* ===================================================================
*/

//#include "pd_database.hpp"
//#include "pd_base_toolkit.hpp"
//#include "pd_base_toolkit_2.hpp"
//#include "gsRMShellAssembler.h"
#include "gsRMShellAssembler.hpp"
#include "pd_solver_beampd.hpp"
//#include "gismo.h"

using namespace Eigen;
using namespace gismo;
using namespace DLUT::SAE::PERIDYNAMIC;

//extern TFemMeshCoreData FemMesh;
extern int LOAD_STEP;

template <class T>
class PDIGACouple
{
public:
	// Ĭ�Ϲ��캯��
	PDIGACouple() {	}

	// >>> ======================================================================
	// ��ʼ�� PD ��Ϣ�����캯����
	PDIGACouple(gsRMShellAssembler<T>* igaAss,
		const gsNURBSinfo<T>& nurbsinfo,
		Material m_material)
		:m_nurbsinfo(nurbsinfo), substance(m_material)
	{
		// 1. ��ʼ��
		m_pd_model.Initialize();

		// 2. ��Lsdyna�ļ��Ķ��룬��ȡ�ڵ����ꡢ��Ԫ�����ϲ�������Ϣ
		// �൱��PD�����е�dyna_seri.ReadLsdynaFile(m_str_lsdyna_file, m_pd_model);
		m_igaAss = igaAss;
		//m_pd_realNodes = pAssem->m_pDNode;//�޸� pAssem��m_pDNode��ֵ��ʹ��ΪPD�����еĵ�
		// m_pd_nodes.Clear();
		initilizePdNodes(m_pd_model);

		//	3. ������Ϣ֮�����PART����Ϣ
		m_pd_model.UpdatePartInfo();

		//	4. Update family information for all PD nodes
		// ���㵥Ԫ�������ڵ�
		m_pd_model.UpdateFamilyInParts();

		//	5. Update initial damage value for all PD nodes
		m_pd_model.GenerateInitDamage();
		//	6. Generate initial crevice and delete the corresponding family information
		m_pd_model.GenerateInitCrevice();
		cout << "Finished to read Pre-Process data." << endl;
	}

	// ��������
	~PDIGACouple() { }

	// >>> ======================================================================
	//2. ��ȡPD���� 
	void initilizePdNodes(TPdModel& pd_model)
	{
		// �ò������귶Χ��ʾ��PD���� [\xi_l,\xi_r]x[\eta_l,\eta_r]
		vector<double> pdDom(4, 0.0);
		if (m_igaAss->m_BoundaryCondition.m_pdDomain.size() != 0)
		{
			pdDom.swap(m_igaAss->m_BoundaryCondition.m_pdDomain);
		}

		// ѭ�����е�patch
		for (int pp = 0; pp < m_nurbsinfo.nurbs_sumpatch; ++pp)
		{
			const gsMatrix<T>& cpNodes = m_nurbsinfo.nurbsdata[pp].phys_coor;
			gsMatrix<T> Parameter_Coor = m_nurbsinfo.nurbsdata[pp].para_coor;
			int node_count = m_nurbsinfo.nurbsdata[pp].patch_sumcp;	// ���Ƶ�����

			// ���ڻ���PD�����������������
			double crosswise_span = m_nurbsinfo.nurbsdata[pp].knot_vector[0].knotvec_span; // ����ڵ��
			double lengthway_span = m_nurbsinfo.nurbsdata[pp].knot_vector[1].knotvec_span; // ����ڵ��

			// �õ���PD����&������򣩵�PD��
			// 1. ��ȡ���Ƶ��ż������� *NODE 
			gsMatrix<T> Anch_Coor = m_nurbsinfo.nurbsdata[pp].anch_coor;
			for (int i = 0; i < node_count; ++i)
			{
				// ���� PD �� 
				if (m_igaAss->m_BoundaryCondition.m_pdDomain.size() != 0
					&& ((pdDom[0] - 4 * crosswise_span) <= Anch_Coor(0, i)) && (Anch_Coor(0, i) <= (pdDom[1] + 4 * crosswise_span))
					&& ((pdDom[2] - 4 * lengthway_span) <= Anch_Coor(1, i)) && (Anch_Coor(1, i) <= (pdDom[3] + 4 * lengthway_span)))
				{
					Vector3d tem(cpNodes(i, 0), cpNodes(i, 1), cpNodes(i, 2));
					pd_model.PdMeshCore().InsertNode(tem, i);
					map_fem_nid_global_to_fem_nid_local.insert(
						pair<int, int>(i, pd_model.PdMeshCore().NodeCount() - 1));
					
					// ��ΪPd�ն����ά�ȱ�С�ˣ���Ҫ��һ���ֲ���ȫ�ֵ�ӳ���ϵ���Ա�����װ����ն���
					map_pd_nid_local_to_pd_nid_global.insert(
						pair<int, int>(pd_model.PdMeshCore().NodeCount() - 1, i));
				}
			}

			// 2. ��ȡ��Ԫ��ż���Ԫ�ڵ��� *ELEMENT_SHELL
			unsigned int jj = 0;
			int ele_row = m_nurbsinfo.nurbsdata[pp].knot_vector[0].knotvec_size - 1;
			int ele_col = m_nurbsinfo.nurbsdata[pp].knot_vector[1].knotvec_size - 1;

			vector<int> seq = m_nurbsinfo.nurbsdata[pp].cp_no;// ���Ƶ���
			for (unsigned j = 0; j < ele_row; ++j)
			{
				for (unsigned k = 0; k < ele_col; ++k)
				{
					int st = k + (ele_row + 1) * j;

					int eid = jj;			// ��Ԫ���
					int pid = pp + 1;		// patch���
					int n1 = seq[st];		// ��Ԫ�ڵ���
					int n2 = seq[st + 1];
					int n3 = seq[st + 2 + ele_row];
					int n4 = seq[st + 1 + ele_row];

					// ���� PD �� 
					if (m_igaAss->m_BoundaryCondition.m_pdDomain.size() != 0
						&& ((pdDom[0] - 4 * crosswise_span) <= Anch_Coor(0, n1)) && (Anch_Coor(0, n3) <= (pdDom[1] + 4 * crosswise_span))
						&& ((pdDom[2] - 4 * lengthway_span) <= Anch_Coor(1, n2)) && (Anch_Coor(1, n4) <= (pdDom[3] + 4 * lengthway_span)))
					{
						vector<int> nids;
						nids.push_back(map_fem_nid_global_to_fem_nid_local[n1]);
						nids.push_back(map_fem_nid_global_to_fem_nid_local[n2]);
						nids.push_back(map_fem_nid_global_to_fem_nid_local[n3]);
						nids.push_back(map_fem_nid_global_to_fem_nid_local[n4]);

						pd_model.PdMeshCore().InsertElement(nids, eid, pid);
						map_fem_eid_global_to_fem_eid_local.insert(
							pair<int, int>(eid, pd_model.PdMeshCore().ElementCount() - 1));

						// ���ô�PD����ɵĵ�Ԫ
						/*if (((pdDom[0] - crosswise_span) <= Anch_Coor(0, n1)) && (Anch_Coor(0, n3) <= (pdDom[1] + crosswise_span))
							&& ((pdDom[2] - lengthway_span) <= Anch_Coor(1, n2)) && (Anch_Coor(1, n4) <= (pdDom[3] + lengthway_span)))*/
							if ((pdDom[0] <= Anch_Coor(0, n1)) && (Anch_Coor(0, n3) <= pdDom[1] )
								&& (pdDom[2] <= Anch_Coor(1, n2)) && (Anch_Coor(1, n4) <= pdDom[3] ))
						{
							m_pd_realElement.insert(eid);
							pd_model.PdMeshCore().InsertRealElement(eid);
						}
					}

					++jj;
				}
			}
		}// end of patch for-loop

		// 3. ��ȡ�������� *MAT_ELASTIC *PART 
		int mid = 1;
		double density = substance.rho;
		double elastic_modulus = substance.E_modulus;
		double poisson_ratio = substance.poisson_ratio;
		double stress_tensile = 0;
		double stress_compressive = 0;
		TMaterial mat;
		mat.Id() = mid;
		mat.Name() = "MAT_ELASTIC";
		mat.InsertMatData("Rho", density);
		mat.InsertMatData("E", elastic_modulus);
		mat.InsertMatData("PR", poisson_ratio);
		mat.InsertMatData("STRESS_TENSILE", stress_tensile);
		mat.InsertMatData("STRESS_COMPRESSIVE", stress_compressive);
		pd_model.AddMaterial(mat);

		// 4. *SECTION_SHELL
		int pid = 1;		//	SECTION_ID
		int ELEFORM = 0;
		double THICKNESS = substance.thickness;
		TSection sec(pid);
		if (ELEFORM == 13)
		{
			sec.Type() = string("PLANE_STRAIN");
		}
		else
		{
			sec.Type() = string("PLANE_STRESS");
		}
		sec.InsertSectionData("THICKNESS", THICKNESS);
		pd_model.AddSection(sec);

		// 5. *PART
		int partid = 1;
		int propid = 1;
		int matid = 1;
		TPart part;
		part.Id() = partid;
		part.MaterialId() = matid;
		part.SectionId() = propid;
		pd_model.AddPart(part);


		// 6. ����PD����������е�PD��
		//for (int i = 0; i < m_nurbsinfo.nurbs_sumcp; ++i)
		//{
		//	m_pd_allnodes_set.insert(i);
		//}
		//m_Pd_disp.setZero(m_nurbsinfo.nurbs_sumcp * 6, 1);
		//m_Pd_vel.setZero(m_nurbsinfo.nurbs_sumcp * 3, 1);
		//m_Pd_acc.setZero(m_nurbsinfo.nurbs_sumcp * 3, 1);

		gsInfo << "\nPd node initialization start... ";
		// �õ�PD�����PD��
		int local_id = 0;
		for (int pp = 0; pp < m_nurbsinfo.nurbs_sumpatch; ++pp)
		{
			gsMatrix<T> Anch_Coor = m_nurbsinfo.nurbsdata[pp].anch_coor;
			for (int i = 0; i < m_nurbsinfo.nurbsdata[pp].patch_sumcp; ++i)
			{
				local_id = i; // �ȿ��ǵ�Ƭ�����
				// ���� PD �� 
				if (m_igaAss->m_BoundaryCondition.m_pdDomain.size() != 0
					&& (Anch_Coor(0, i) >= pdDom[0]) && (Anch_Coor(0, i) <= pdDom[1])
					&& (Anch_Coor(1, i) >= pdDom[2]) && (Anch_Coor(1, i) <= pdDom[3]))
				{
					m_pd_realNodes.insert(local_id);
				}
			}
		}
		gsInfo << "done.\n" << endl;
	}

	// >>> ======================================================================
	// 7. ��ʽ��� 
	void ImplicitAnalysis()
	{
		// ����΢ģ��
		pd_solver.Attach(m_pd_model);

		// ����ն���
		for (int cur_step = 1; cur_step <= LOAD_STEP; ++cur_step)
		{
			pd_solver.genGlobalStiffnessPD();
		}
	}

	// >>> ======================================================================
	// 8. ��PD�ն�����IGA�ն������ 
	void  combineStiffWithIGA()
	{
		if (m_pd_realNodes.size() == 0)
		{
			return;
		}

		//���ȼ��θն����ӦPD�����������Ϊ��
		// ĳ�У����Ƶ���*���ɶȣ�
		for (int k = 0; k < m_igaAss->m_system.matrix().outerSize(); ++k)
		{
			// ĳ�У������ݲ�Ϊ����У�
			for (typename SparseMatrix<T>::InnerIterator it(m_igaAss->m_system.matrix(), k); it; ++it)
			{
				if (m_pd_realNodes.find(it.row() / 6) != m_pd_realNodes.end())
				{
					// gsDebug << "set as Zero for row "  << it.row() << "\t" << it.col() << endl;
					// pm_PlaneIGAAssembler->m_matrix.coeffRef(it.row(), it.col() ) = 0;
					it.valueRef() = 0;
				}
			}
		}

		// ����������ĸն���
		/*
		ofstream igaok("../../TestResult/RM_Shell_row0k.txt");
		for (index_t i = 0; i < m_igaAss->m_system.matrix().rows(); ++i)
		{
			for (index_t j = 0; j < m_igaAss->m_system.matrix().cols(); ++j)
			{
				igaok << setw(16) << m_igaAss->m_system.matrix().coeffRef(i, j);
			}
			igaok << endl;
		}
		igaok.close();*/

		gsSparseMatrix<double> sparse_matrix_GK;
		TransVecMap2SparseMatrix(pd_solver.m_GK, sparse_matrix_GK);
		//gs_sparsematrix_GK = gsSparseMatrix<double>(sparse_matrix_GK);

		/*���滻�ĺ��Ĵ���*/
		// 1.����PD�ն������е���
		// 2.ѡȡ����PD�ն��������
		// 3.ȷ�����ж�ӦPD���Ƿ�Ϊ��PD��
		// 4.��ֵ�����ȼ��θն�����
		for (int i = 0; i < sparse_matrix_GK.outerSize(); ++i)
		{
			for (typename SparseMatrix<T>::InnerIterator it2(sparse_matrix_GK, i); it2; ++it2)
			{
				int igaj = map_pd_nid_local_to_pd_nid_global[it2.row() / 6];
				int igak = map_pd_nid_local_to_pd_nid_global[it2.col() / 6];
				if (m_pd_realNodes.find(igaj) != m_pd_realNodes.end())
				{
					int igarow = igaj*6 + it2.row() % 6;
					int igacol = igak*6 + it2.col() % 6;
					m_igaAss->m_system.matrix().coeffRef(igarow, igacol) = it2.value();
				}
			}
		}
		/*parallel_
		for_each(m_pd_realNodes.begin(), m_pd_realNodes.end(), [&](int ni)
			{
				int dof = 6;
				int pdi = map_fem_nid_global_to_fem_nid_local[ni]; // ��ȡiga���Ŷ�Ӧ�ľֲ�PD���

				// ����pd��ĵȼ��θն��������� ni * dof + alli 
				for (int alli = 0; alli < dof; ++alli)
				{

					// ����pd���pd�ն��������� nj * dof + allj
					for (int nj=0; nj < map_pd_nid_local_to_pd_nid_global.size(); ++nj)
					{
						// ��ȡpd���Ŷ�Ӧ��ȫ�ֱ��
						int igaj = map_pd_nid_local_to_pd_nid_global[nj];

						for (int allj=0; allj<dof; ++allj)
						{
							m_igaAss->m_system.matrix().coeffRef(ni * dof + alli, igaj * dof + allj)
								= sparse_matrix_GK(pdi*dof+alli, nj * dof + allj);
						}
						
					}
				}
			});*/

		// gsOutputSparseMatrix(pm_PlaneIGAAssembler->m_matrix, "stiffness5.txt");

		// ������滻��ĸն���
		/*
		ofstream couk("../../TestResult/RM_Shell_couk.txt");
		for (index_t i = 0; i < m_igaAss->m_system.matrix().rows(); ++i)
		{
			for (index_t j = 0; j < m_igaAss->m_system.matrix().cols(); ++j)
			{
				couk << setw(16) << m_igaAss->m_system.matrix().coeffRef(i, j);
			}
			couk << endl;
		}
		couk.close();*/

		// m_igaAss->m_system.matrix().makeCompressed(); // �ǲ���ѹ����
	}

	// >>> ======================================================================
	//��ϸն���
	void _combineStiff(gsSparseMatrix<T>& out, const int rout,
		gsSparseMatrix<T> inm, const int rin)
	{
		//��ֵ ��in��ֵ����out
		for (typename Eigen::SparseMatrix<T, Eigen::RowMajor>::InnerIterator it2(inm, rin); it2; ++it2)
		{
			out.coeffRef(rout, it2.col()) = it2.value();
		}
		// ��ȷ������Ч���ر�ͣ��������ַ�����
	   /* int nodeNm = out.cols()/6;
		for (int i=0; i<nodeNm; ++i)
		{
			for (int j=0; j< 6; ++j)
			{
				out(rout, i*6+j) = inm(rin,i*3+j);
			}
		 }*/
	}

	// >>> ======================================================================
public:
	TPdModel				m_pd_model;		//	PD data of everything
	gsNURBSinfo<T>			m_nurbsinfo;	// ���β�����Ϣ
	gsRMShellAssembler<T>*  m_igaAss;		// �ȼ�����Ϣ����
	//�洢���еĿ��Ƶ���ΪPD��
	// TPdNodeCollector<T> m_pd_nodes;
	//ָ��ת��ΪPD��Ŀ��Ƶ�
	set<int>				m_pd_realNodes;
	set<int>				m_pd_realElement;	// ʵ��pd�㹹�ɵĵ�Ԫ
	set<int>				m_pd_allnodes_set;
	//	FEM NODE GLOBAL    <-> FEM NODE LOCAL
	map<int, int>			map_fem_nid_global_to_fem_nid_local;
	map<int, int>			map_pd_nid_local_to_pd_nid_global;
	//	FEM ELEMENT GLOBAL <-> FEM ELEMENT LOCAL
	map<int, int>			map_fem_eid_global_to_fem_eid_local;

	TSolverBeamPD			pd_solver;
	Material				substance;
public:
	//PD��İ뾶
	int						m_iterator_nums; //��ѭ������
	double					PD_Radius;
	double					m_time_interval; //����
	double					m_time_total;
	gsMatrix<T>				m_Pd_disp;
	gsMatrix<T>				m_Pd_vel;
	gsMatrix<T>				m_Pd_acc;
	gsSparseMatrix<double>	gs_sparsematrix_GK;
};

