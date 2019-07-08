#include <AMReX.H>
#include <AMReX_Print.H>

#include <IBMarkerMD.H>



namespace immbdy_geom {

Vertex::Vertex(const RealVect & r_in) : r(r_in) {

    Clear();
}



void Vertex::Clear() {

    f = RealVect{AMREX_D_DECL(0., 0., 0.)};
}



Edge::Edge(Vertex & v1, Vertex & v2, Real length_0)
    : m_start(v1), m_end(v2), m_length_0(length_0) {

    Update();
}



Edge::Edge(Vertex & v1, Vertex & v2) : m_start(v1), m_end (v2) {

    RealVect link_init = m_end.r - m_start.r;
    m_length_0 = link_init.vectorLength();

    Update();
}



void Edge::Update() {

    // Construct link vector
    m_link = m_end.r - m_start.r;

    // Edge length
    m_length = m_link.vectorLength();

    // Construct normal vector
    m_normal = m_link;
    if (m_length > 0)
        m_normal *= 1./m_length;
}

};



namespace immbdy_md {

using namespace immbdy_geom;


void add_bending_forces(Edge & e_ref, Edge & e, Real k, Real cos_theta_0) {


    //___________________________________________________________________________
    // edge lengths (and squares)

    Real l_ref = e_ref.length();
    Real l_e   = e.length();

    Real l2M = l_ref*l_ref;
    Real l2P = l_e*l_e;


    //___________________________________________________________________________
    // r, r_p, r_m <= vectors representing the central, previous (m) and next (p)
    //                vertex positions

    const RealVect & r = e.start().r;
    const RealVect & r_p = e.end().r;
    const RealVect & r_m = e_ref.start().r;


    //___________________________________________________________________________
    // angle between two edges

    Real cos_theta = e_ref.normal().dotProduct(e.normal());


    //___________________________________________________________________________
    // cartesian coordinates of vertices (m: previous, p: next)

    Real x = r[0], xM = r_m[0], xP = r_p[0];
#if (AMREX_SPACEDIM > 1)
    Real y = r[1], yM = r_m[1], yP = r_p[1];
#endif
#if (AMREX_SPACEDIM > 2)
    Real z = r[2], zM = r_m[2], zP = r_p[2];
#endif


    //___________________________________________________________________________
    // compute bending forces analytically

    Real fx2 = -2*(-cos_theta + cos_theta_0)*((cos_theta*(x - xM))/l2M
            + (cos_theta*(x - xP))/l2P - (-2*x + xM + xP)/(l_ref*l_e));

    Real fPx2 = -2*(-cos_theta + cos_theta_0)*((-x + xM)/(l_ref*l_e)
            + (cos_theta*(-x + xP))/l2P);

    Real fMx2 = -2*(-cos_theta + cos_theta_0)*((cos_theta*(-x + xM))/l2M
            + (-x + xP)/(l_ref*l_e));


#if (AMREX_SPACEDIM > 1)
    Real fy2 = -2*(-cos_theta + cos_theta_0)*((cos_theta*(y - yM))/l2M
            + (cos_theta*(y - yP))/l2P - (-2*y + yM + yP)/(l_ref*l_e));

    Real fPy2 = -2*(-cos_theta + cos_theta_0)*((-y + yM)/(l_ref*l_e)
            + (cos_theta*(-y + yP))/l2P);

    Real fMy2 = -2*(-cos_theta + cos_theta_0)*((cos_theta*(-y + yM))/l2M
            + (-y + yP)/(l_ref*l_e));
#endif

#if (AMREX_SPACEDIM > 2)
    Real fz2 = -2*(-cos_theta + cos_theta_0)*((cos_theta*(z - zM))/l2M
            + (cos_theta*(z - zP))/l2P - (-2*z + zM + zP)/(l_ref*l_e));

    Real fPz2 = -2*(-cos_theta + cos_theta_0)*((-z + zM)/(l_ref*l_e)
            + (cos_theta*(-z + zP))/l2P);

    Real fMz2 = -2*(-cos_theta + cos_theta_0)*((cos_theta*(-z + zM))/l2M
            + (-z + zP)/(l_ref*l_e));
#endif


    //___________________________________________________________________________
    // Add bending forces to the vertex data

    RealVect f   = RealVect(AMREX_D_DECL(k*fx2/2,  k*fy2/2,  k*fz2/2));
    RealVect f_p = RealVect(AMREX_D_DECL(k*fPx2/2, k*fPy2/2, k*fPz2/2));
    RealVect f_m = RealVect(AMREX_D_DECL(k*fMx2/2, k*fMy2/2, k*fMz2/2));

    e.start_f()     += f;
    e.end_f()       += f_p;
    e_ref.start_f() += f_m;
}



void bending_f(      RealVect & f,       RealVect & f_p,       RealVect & f_m,
               const RealVect & r, const RealVect & r_p, const RealVect & r_m,
               Real k, Real cos_theta_0) {

    // Construct vertices (note that Vertex::f == 0 due to constructor)
    Vertex v(r), v_p(r_p), v_m(r_m);

    // Set up topology
    Edge e_ref(v_m, v), e(v, v_p);

    // Add bending forces to vertices
    add_bending_forces(e_ref, e, k, cos_theta_0);


    // Add result to bending forces to output variables
    f   += e.start().f;
    f_p += e.end().f;
    f_m += e_ref.start().f;
}

};