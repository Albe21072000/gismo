/** @file gsGeometryEvaluator.hpp

    @brief Provides implementation of GeometryEvaluation interface.
    
    This file is part of the G+Smo library-
    
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): A. Bressan, C. Hofreither, J. Sogn, A. Mantzaflaris
*/

namespace gismo
{

template <unsigned d, typename T>
struct jacobianPartials 
{ 
    static void 
    compute(const typename gsMatrix<T>::constColumn  & secDer,
            std::vector<gsMatrix<T> > & DJac)
    {
        // Number of second derivatives
        //const index_t n2d = (ParDim + (ParDim*(ParDim-1))/2);
        GISMO_ERROR("Not implemented.");
    }
};

template <typename T>
struct jacobianPartials<2,T>
{
    static void 
    compute(const typename gsMatrix<T>::constColumn  & secDer,
            std::vector<gsMatrix<T> > & DJac)
    {
        DJac.resize(2, gsMatrix<T>(2,2) );
        // Note: stride is 3
        //dx
        DJac[0](0,0) = secDer(0);//G1xx
        DJac[0](0,1) = secDer(2);//G1xy
        DJac[0](1,0) = secDer(3);//G2xx
        DJac[0](1,1) = secDer(5);//G2xy
        //dy
        DJac[1](0,0) = secDer(2);//G1yx
        DJac[1](0,1) = secDer(1);//G1yy
        DJac[1](1,0) = secDer(5);//G2yx
        DJac[1](1,1) = secDer(4);//G2yy
    }
};

template <typename T>
struct jacobianPartials<3,T>
{
    static void 
    compute(const typename gsMatrix<T>::constColumn  & secDer,
            std::vector<gsMatrix<T> > & DJac)
    {
        DJac.resize(3, gsMatrix<T>(3,3) );
        // Note: stride is 6
        //du
        DJac[0](0,0) = secDer(0 );//G1uu
        DJac[0](0,1) = secDer(3 );//G1uv
        DJac[0](0,2) = secDer(4 );//G1uw
        DJac[0](1,0) = secDer(6 );//G2uu
        DJac[0](1,1) = secDer(9 );//G2uv
        DJac[0](1,2) = secDer(10);//G2uw
        DJac[0](2,0) = secDer(12);//G3uu
        DJac[0](2,1) = secDer(15);//G3uv
        DJac[0](2,2) = secDer(16);//G3uw
        //dv
        DJac[1](0,0) = secDer(3 );//G1vu
        DJac[1](0,1) = secDer(1 );//G1vv
        DJac[1](0,2) = secDer(5 );//G1vw
        DJac[1](1,0) = secDer(9 );//G2vu
        DJac[1](1,1) = secDer(7 );//G2vv
        DJac[1](1,2) = secDer(11);//G2vw
        DJac[1](2,0) = secDer(15);//G3vu
        DJac[1](2,1) = secDer(13);//G3vv
        DJac[1](2,2) = secDer(17);//G3vw
        //dw
        DJac[2](0,0) = secDer(4 );//G1wu
        DJac[2](0,1) = secDer(5 );//G1wv
        DJac[2](0,2) = secDer(2 );//G1ww
        DJac[2](1,0) = secDer(10);//G2wu
        DJac[2](1,1) = secDer(11);//G2wv
        DJac[2](1,2) = secDer(8 );//G2ww
        DJac[2](2,0) = secDer(16);//G3wu
        DJac[2](2,1) = secDer(17);//G3wv
        DJac[2](2,2) = secDer(14);//G3ww
    }
};


// Geometry transformation 
template <class T, int ParDim, int GeoDim>
struct gsGeoTransform
{
    // Integral transformation for volume integrals
    static void getVolumeElements( gsMatrix<T> const & jacobians,
                                   gsVector<T> & result)
    {
        const index_t numPts = jacobians.cols() / ParDim;
        result.resize(numPts);

        for (index_t i = 0; i < numPts; ++i)
        {
            const Eigen::Block<const typename gsMatrix<T>::Base,GeoDim,ParDim> Ji =
                    jacobians.template block<GeoDim,ParDim>(0, i * ParDim);

            result[i] = math::sqrt( (Ji.transpose() * Ji).determinant() );
        }
    }

    // Transformation for parametric gradient
    static void getGradTransform( gsMatrix<T> const & jacobians,
                                  gsMatrix<T> & result)
    {
        const index_t numPts = jacobians.cols() / ParDim;
        result.resize(GeoDim, numPts * ParDim);

        for (index_t i = 0; i < numPts; ++i)
        {
            const Eigen::Block<const typename gsMatrix<T>::Base,GeoDim,ParDim> Ji =
                    jacobians.template block<GeoDim,ParDim>(0, i * ParDim);
            
            result.template block<GeoDim,ParDim>(0, i*ParDim) =
                Ji *  ( Ji.transpose() * Ji ).inverse().eval(); // temporary here
        }
    }

    // Laplacian transformation for parametric second derivatives
    static void getLaplTransform(index_t k,
                                 gsMatrix<T> const & jacobians,
                                 const typename gsMatrix<T>::Block allDerivs2,
                                 gsMatrix<T> & result)
    {
        //
    }

    // Outer (co-)normal vector at point k
    static void getOuterNormal(index_t k,
                               gsMatrix<T> const & jacobians,
                               boxSide s,
                               gsMatrix<T> & result)
    {
        // Assumes points k on boundary "s"
        // Assumes codim = 0 or 1

        const T   sgn = sideOrientation(s); // (!) * m_jacSign;
        const int dir = s.direction();

        result.resize(GeoDim);

        const gsMatrix<T, GeoDim, ParDim> Jk =
                jacobians.template block<GeoDim,ParDim>(0, k*ParDim);

        T alt_sgn = sgn;

        gsMatrix<T, ParDim-1, ParDim-1> minor;
        for (int i = 0; i != GeoDim; ++i) // for all components of the normal
        {
            Jk.firstMinor(i, dir, minor);
            result[i] = alt_sgn * minor.determinant();
            alt_sgn  *= -1;
        }

        /*
        // tangent vector
        const gsVector<T,GeoDim> tangent  = sgn * jacobians.template block<GeoDim, 1>(0,!dir);
        if ( tangent.squaredNorm() < 1e-10 )
        gsWarn<< "Zero tangent.\n";
        
        // Manifold outer normal vector
        const gsVector<T,3> outer_normal = jacobians.template block<GeoDim, 1>(0,0).cross(
        jacobians.template block<GeoDim, 1>(0,1) ).normalized();
        
        // Co-normal vector
        result = tangent.cross( outer_normal ) ;
        */
    }

    static void getNormal(index_t k,
                          gsMatrix<T> const & jacobians,
                          boxSide s,
                          gsMatrix<T> & result)
    {
        //
    }
};

// Geometry transformation : Specialization for co-dimension zero
template <class T, int ParDim>
struct gsGeoTransform<T,ParDim,ParDim>
{
    // Integral transformation for volume integrals
    static void getVolumeElements(gsMatrix<T> const & jacobians,
                                  gsVector<T> & result)
    {
        const index_t numPts = jacobians.cols() / ParDim;
        result.resize(numPts);

        for (index_t i = 0; i < numPts; ++i)
        {
            result[i] =
                    math::fabs((jacobians.template block<ParDim,ParDim>(0, i*ParDim)).determinant());
        }
    }

    // Transformation for parametric gradient
    static void getGradTransform( gsMatrix<T> const & jacobians,
                                  gsMatrix<T> & result)
    {
        const index_t numPts = jacobians.cols() / ParDim;
        result.resize(ParDim, numPts * ParDim);

        for (index_t i = 0; i < numPts; ++i)
        {
            result.template block<ParDim,ParDim>(0, i*ParDim) =
                    jacobians.template block<ParDim,ParDim>(0, i*ParDim).inverse().transpose();
        }
    }

    // Laplacian transformation at allDerivs2.col(k)
    static void getLaplTransform(index_t k,
                                 gsMatrix<T> const & jacobians,
                                 const typename gsMatrix<T>::Block & allDerivs2,
                                 gsMatrix<T> & result)
    {
        //
    }

    // Outer (co-)normal vector at point k
    static void getOuterNormal(index_t k,
                               gsMatrix<T> const & jacobians,
                               boxSide s,
                               gsMatrix<T> & result)
    {
        // Assumes points k on boundary "s"
        // Assumes codim = 0 or 1

        const T   sgn = sideOrientation(s); // (!) * m_jacSign;
        const int dir = s.direction();

        result.resize(ParDim);

        const gsMatrix<T, ParDim, ParDim> & Jk =
                jacobians.template block<ParDim,ParDim>(0, k*ParDim);

        gsMatrix<T, ParDim-1, ParDim-1> minor;
        T alt_sgn = sgn;
        for (int i = 0; i != ParDim; ++i) // for all components of the normal
        {
            Jk.firstMinor(i, dir, minor);
            result[i] = alt_sgn * minor.determinant();
            alt_sgn  *= -1;
        }
    }

    static void getNormal(index_t k,
                          gsMatrix<T> const & jacobians,
                          boxSide s,
                          gsMatrix<T> & result)
    {
        //
    }

};


//////////////////////////////////////////////////
//////////////////////////////////////////////////


template <class T, int ParDim, int codim>
void
gsGenericGeometryEvaluator<T,ParDim,codim>::evaluateAt(const gsMatrix<T>& u)
{
    GISMO_ASSERT( m_maxDeriv != -1, "Error in evaluation flags. -1 not supported yet.");

    m_numPts = u.cols();
    m_geo.basis().evalAllDers_into(u, m_maxDeriv, m_basisVals);
    // todo: If we assume all points (u) lie on the same element we may
    // store the actives only once
    m_geo.basis().active_into(u, m_active);
    
    if (this->m_flags & NEED_VALUE)
        computeValues();
    if (this->m_flags & NEED_JACOBIAN)
        computeJacobians();
    if (this->m_flags & NEED_MEASURE)
        gsGeoTransform<T,ParDim,GeoDim>::getVolumeElements(m_jacobians, m_measures);
    if (this->m_flags & NEED_GRAD_TRANSFORM)
        gsGeoTransform<T,ParDim,GeoDim>::getGradTransform(m_jacobians, m_jacInvs);
    if (this->m_flags & NEED_2ND_DER)
        compute2ndDerivs();
/*
    if (this->m_flags & NEED_DIV)
        divergence(m_div);
    if (this->m_flags & NEED_CURL)
        computeCurl();
    if (this->m_flags & NEED_LAPLACIAN)
        computeLaplacian();
    if (this->m_flags & NEED_NORMAL)
        computeNormal();
*/
}


template <class T, int ParDim, int codim>
void gsGenericGeometryEvaluator<T,ParDim,codim>::computeValues()
{
    const gsMatrix<T> & coefs = m_geo.coefs();

    m_values.resize(coefs.cols(), m_numPts);

    for (index_t j=0; j < m_numPts; ++j) // for all evaluation points
    {
        m_values.col(j) =  coefs.row( m_active(0,j) ) * m_basisVals(0,j);

        for ( index_t i=1; i< m_active.rows() ; i++ )   // for all non-zero basis functions
            m_values.col(j)  +=   coefs.row( m_active(i,j) ) * m_basisVals(i,j);
    }
}

template <class T, int ParDim, int codim>
void gsGenericGeometryEvaluator<T,ParDim,codim>::computeJacobians()
{
    const index_t numActive = m_active.rows();
    const gsMatrix<T> & coefs = m_geo.coefs();

    m_jacobians.setZero(GeoDim, m_numPts * ParDim);

    for (index_t j = 0; j < m_numPts; ++j)
        for (index_t i = 0; i < numActive; ++i) // for all active basis functions
        {
            m_jacobians.template block<GeoDim,ParDim>(0,j*ParDim).transpose().noalias() +=
                    m_basisVals.template block<ParDim,1>(numActive+i*ParDim, j) *
                    coefs.template block<1,GeoDim>(m_active(i,j),0) ;

            // to check: which is faster ? the above or this..
            // m_jacobians.template block<GeoDim,ParDim>(0,j*ParDim).noalias() +=
            //     (m_basisVals.template block<ParDim,1>(numActive+i*ParDim, j)
            //      * coefs.template block<1,GeoDim>(m_active(i,j),0) ).transpose();
        }
}


template <class T, int ParDim, int codim>
void gsGenericGeometryEvaluator<T,ParDim,codim>::compute2ndDerivs()
{
    const index_t numActive = m_active.rows();
    const gsMatrix<T> & coefs = m_geo.coefs();
    // Number of differnt 2ed derivativs combinations
    const index_t numDeriv = ParDim + (ParDim*(ParDim - 1))/2;
    // Starting index of the second derivatives in m_basisVals
    const index_t der2start = (1 + ParDim) * numActive;

    m_2ndDers.setZero(GeoDim*numDeriv, m_numPts);

    gsMatrix<T> reshape; // to be removed later

    for (index_t j = 0; j < m_numPts; ++j)
    {
        for (index_t i = 0; i < numActive; ++i) // for all active basis functions
        {
            reshape.noalias() =
                    m_basisVals.template block<numDeriv,1>(der2start+i*numDeriv, j)
                    * coefs.template block<1,GeoDim>(m_active(i,j),0);

            reshape.resize(GeoDim*numDeriv,1);
            m_2ndDers.template block<GeoDim*numDeriv,1>(0,j).noalias() += reshape;
        }
    }
}


template <class T, int ParDim, int codim> // AM: Not yet tested
void gsGenericGeometryEvaluator<T,ParDim,codim>::
transformValuesHdiv( index_t k,
                     const std::vector<gsMatrix<T> >& allValues,
                     gsMatrix<T>  & result) const
{
    GISMO_ASSERT( static_cast<size_t>(GeoDim) == allValues.size(), "Expecting TarDim==GeoDim.");
    int numA = 0;
    for(int comp = 0; comp < GeoDim; ++comp)
        numA += allValues[comp].rows();
    result.resize(GeoDim,numA); // GeoDim x numA
    
    const T det = this->jacDet(k);
    const typename gsMatrix<T>::constColumns & jac = this->jacobian(k);    
    index_t c = 0;
    for(int comp = 0; comp < GeoDim; ++comp)  // component
    {
        const typename gsMatrix<T>::constColumn & bvals = allValues[comp].col(k);
        for( index_t j=0; j< allValues[comp].rows() ; ++j) // active of component
        {
            result.col(c++) = ( bvals[j] / det ) * jac.col(comp);
        }
    }
}

template <class T, int ParDim, int codim>  // AM: Not yet tested
void gsGenericGeometryEvaluator<T,ParDim,codim>::
transformGradsHdiv( index_t k,
                    const std::vector<gsMatrix<T> >& allValues,
                    std::vector<gsMatrix<T> > const & allGrads,
                    gsMatrix<T> & result) const
{
/*
    //Assumptions: GeoDim = ParDim = TargetDim
    
    index_t c = 0;
    for(size_t comp = 0; comp < allValues.size(); ++comp)
        c += allValues[c].rows();
    result.resize(GeoDim*ParDim,c);
    
    const T det = this->jacDet(k);
    const typename gsMatrix<T>::constColumn  & secDer = this->deriv2(k);
    const typename gsMatrix<T>::constColumns & Jac    = this->jacobian(k);
    const Eigen::Transpose< const typename gsMatrix<T>::constColumns > & 
        invJac = this->gradTransform(k).transpose();
    
    std::vector<gsMatrix<T> > DJac;
    jacobianPartials<ParDim,T>::compute(secDer,DJac);
    
    gsVector<T> gradDetJrec(ParDim);        
    for (int i=0; i<ParDim; ++i)
        gradDetJrec[i] = - ( invJac * DJac[i] ).trace() / det;
    
    c = 0;        
    for(int comp = 0; comp < GeoDim; ++comp) // component
    {
        const typename gsMatrix<T>::constColumn & bvals = allValues[comp].col(k);
        const typename gsMatrix<T>::constColumn & bder  = allGrads [comp].col(k);
        
        for( index_t j=0; j< bvals.rows() ; ++j) // active of component
        {
            gsAsMatrix<T> tGrad(result.col(c).data(), GeoDim, GeoDim);
            
            tGrad.noalias() = Jac.col(comp) * bder.segment(j*ParDim,ParDim).transpose() * invJac / det;
            
            // tGrad.colwise() += gradDetJrec[i];
            for (int i=0; i<ParDim; ++i) // result column
            {
                tGrad.col(i).array()   += gradDetJrec[i];
                
                tGrad.col(i) += ( allValues[comp](j,k) / det ) * DJac[i].col(j);
                }
            
            ++c;// next basis function
        }
    }
//*/
}

//JS2: Verified for the 2D case ("quarte Annulus")
template <class T, int ParDim, int codim>
void gsGenericGeometryEvaluator<T,ParDim,codim>::
transformLaplaceHgrad(  index_t k,
                        const gsMatrix<T> & allGrads,
                        const gsMatrix<T> & allHessians,
                        gsMatrix<T> & result) const
{
    // allgrads
    const index_t numGrads = allGrads.rows() / ParDim;
    // result: 1 X numGrads
    const index_t n2d = (ParDim + (ParDim*(ParDim-1))/2);

    const typename gsMatrix<T>::constColumn  & secDer = this->deriv2(k);

    std::vector<gsMatrix<T> > DJac; // [0]: J_t   [1]: J_s etc...
    std::vector<gsMatrix<T> > DJacPhys(GeoDim); // [0]: J_x   [1]: J_y etc...
    jacobianPartials<ParDim,T>::compute(secDer,DJac);

    //Find the partial derivative wrt the physical cordinates c.
    for(index_t c = 0; c < GeoDim; ++c)
    {
        DJacPhys[c].setZero(GeoDim, ParDim);
        for(index_t j = 0; j < GeoDim; ++j)
        {
            DJacPhys[c] += DJac[j]*m_jacInvs(c,j+k*ParDim);
        }
    }

    gsMatrix<T> jac2inv(GeoDim, GeoDim);
    for(index_t c = 0; c < GeoDim; ++c)
    {
        // - J⁻¹ dJ/dx J⁻¹ = dJ⁻¹/dx
        gsMatrix<T> DJacInvPhys = -m_jacInvs.template block<GeoDim,ParDim>(0, k*ParDim).transpose()
                * DJacPhys[c]*m_jacInvs.template block<GeoDim,ParDim>(0, k*ParDim).transpose();
        jac2inv.row(c) = DJacInvPhys.col(c).transpose();

    }
    //jac2inv contains:
    //(Txx,   Sxx)
    //(Tyy,   Syy)

    const gsAsConstMatrix<T,ParDim> grads_k(allGrads.col(k).data(), ParDim, numGrads);
    gsMatrix<T> collaps;
    collaps.setOnes(1,ParDim);
    result.noalias() = collaps*(jac2inv * grads_k);

    //J⁻¹J⁻T
    gsMatrix<T> tmp = m_jacInvs.template block<GeoDim,ParDim>(0, k*ParDim).transpose()
                    * m_jacInvs.template block<GeoDim,ParDim>(0, k*ParDim);

    gsMatrix<T> hessian(ParDim, ParDim);

    for (index_t i = 0; i < numGrads ; ++i)
    {
        //2D specific case
        if (ParDim == 2)
        {
            hessian(0,0) = allHessians(0+i*n2d,k);
            hessian(1,1) = allHessians(1+i*n2d,k);
            hessian(0,1) = allHessians(2+i*n2d,k);
            hessian(1,0) = allHessians(2+i*n2d,k);
        }
        //3D specific case
        if (ParDim == 3)
        {
            hessian(0,0) = allHessians(0+i*n2d,k);
            hessian(1,1) = allHessians(1+i*n2d,k);
            hessian(2,2) = allHessians(2+i*n2d,k);
            hessian(0,1) = allHessians(3+i*n2d,k);
            hessian(1,0) = allHessians(3+i*n2d,k);
            hessian(0,2) = allHessians(4+i*n2d,k);
            hessian(2,0) = allHessians(4+i*n2d,k);
            hessian(1,2) = allHessians(5+i*n2d,k);
            hessian(2,1) = allHessians(5+i*n2d,k);
        }

        //Frobenius mutiplication of tmp and hessian
        //Exployting symmetry
        for(index_t c = 0; c < GeoDim; ++c)
        {
            result(0,i) += (tmp.row(c)*hessian.col(c)).value();
        }
    }
}


/*
template <class T, int ParDim, int codim>
void gsGenericGeometryEvaluator<T,ParDim,codim>::computeCurl()
{
    /// NOT TESTED FOR CORRECTNESS
    GISMO_ASSERT( ParDim==3 && ParDim == GeoDim, "Implemented only for 3D geometries in 3D spaces");
    const int numPoints=m_jacobians.cols()/ParDim;

    m_curl.resize(GeoDim,numPoints);
    for (int i=0; i<numPoints;++i)
    {
        const typename gsMatrix<T>::constColumns Jac = this->jacobian(i);

        m_curl(0,i)= Jac(2,3) - Jac(3,2);
        m_curl(1,i)= Jac(3,1) - Jac(1,3);
        m_curl(2,i)= Jac(1,2) - Jac(2,1);
    }
}

template <class T, int ParDim, int codim>
void gsGenericGeometryEvaluator<T,ParDim,codim>::computeLaplacian()
{
    /// NOT TESTED FOR CORRECTNESS
    const int numPoints=m_jacobians.cols()/ParDim;
    m_lap.resize(GeoDim,numPoints);
    for (int i=0; i<numPoints;++i)
    {
        m_lap.col(i)=m_2ndDers.template block<ParDim,GeoDim>(0,i*GeoDim).colwise().sum().transpose();
    }
}

template <class T, int ParDim, int codim>
void gsGenericGeometryEvaluator<T,ParDim,codim>::computeNormal()
{
    /// NOT TESTED FOR CORRECTNESS
    GISMO_ASSERT( codim==1, "Codimension should be equal to 1");
    const int numPoints=m_jacobians.cols()/ParDim;
    m_normal.resize(GeoDim,numPoints);
    for (int i=0; i<numPoints;++i)
    {
    // TODO remove the extra copy
        gsVector<T> temp;
        normal(i,temp);
        m_normal.col(i)=temp;
    }
}
*/

}; // namespace gismo
