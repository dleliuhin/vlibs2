#ifndef VSETTINGS_H
#define VSETTINGS_H

#include <memory>
#include <vector>
#include <ostream>
#include "vcat.h"

//=======================================================================================
class vsettings final
{
public:
    using string     = std::string;
    using cstring    = const std::string&;
    using str_vector = std::vector<string>;

    class schema;

    void set( cstring key, cstring val, cstring comment = {} );

    string get( cstring key ) const;

    template<typename T>
    T get( cstring key ) const;

    template<typename T>
    void set( cstring key, const T& val );

    vsettings& subgroup( cstring name, cstring comment = {} );
    const vsettings& subgroup( cstring name ) const;

    bool has_key      ( cstring key )  const;
    bool has_subgroup ( cstring name ) const;

    str_vector keys()      const;
    str_vector subgroups() const;

    string to_ini() const;
    void from_ini( cstring ini );

    void from_ini_file( cstring fname );
    void to_ini_file  ( cstring fname ) const;

    vsettings();
    ~vsettings();

    cstring comment_of_key      ( cstring key  ) const;
    cstring comment_of_subgroup ( cstring name ) const;

    static bool is_valid_key      ( cstring key  );
    static bool is_valid_subgroup ( cstring name );

    vsettings(const vsettings&) = default;
    vsettings& operator =(const vsettings&) = default;

private:
    class _pimpl; std::shared_ptr<_pimpl> _p;
};
//=======================================================================================
class vsettings::schema final
{
public:
    template<typename T>
    void add( cstring key, T *val, cstring comment = {} );

    void capture( const vsettings& settings );
    void capture_from_ini( cstring fname );

    vsettings build() const;
    void save_to_ini( cstring fname ) const;

    void subgroup( cstring name, cstring comment = {} );
    void end_subgroup();

private:
    //  Развязка для аккуратного хранения типизированных указателей на данные.
    struct _node_iface;
    using  _node_ptr = std::shared_ptr<_node_iface>;
    template <typename T> struct _node;
    void _add_node( _node_ptr && );

    std::vector<_node_ptr> _nodes;

    struct _group_t
    {
        using vector = std::vector<_group_t>;
        string name, comment;
    };
    _group_t::vector _groups;
    friend bool operator == ( const _group_t::vector& lhs, const _group_t::vector& rhs );
};
//=======================================================================================


//=======================================================================================
//      Implementation
//=======================================================================================
std::ostream& operator << (std::ostream& os, const vsettings& sett );
//=======================================================================================
template<typename T>
T vsettings::get( cstring key ) const
{
    return vcat::from_text<T>( get(key) );
}
//=======================================================================================
template<typename T>
void vsettings::set( cstring key, const T& val )
{
    set( key, vcat(val).str() );
}
//=======================================================================================


//=======================================================================================
struct vsettings::schema::_node_iface
{
    string key, comment;
    _group_t::vector groups;

    _node_iface( cstring k, cstring c )
        : key     ( k )
        , comment ( c )
    {}

    virtual void* stored_ptr() const                = 0;

    virtual void load( const vsettings& settings )  = 0;
    virtual void save( vsettings* settings ) const  = 0;

    virtual ~_node_iface();
};
//=======================================================================================
template <typename T>
struct vsettings::schema::_node : vsettings::schema::_node_iface
{
    //-----------------------------------------------------------------------------------
    T* ptr;
    //-----------------------------------------------------------------------------------
    _node( cstring k, cstring c, T *p )
        : _node_iface( k, c )
        , ptr( p )
    {}
    //-----------------------------------------------------------------------------------
    virtual void* stored_ptr() const override
    {
        return ptr;
    }
    //-----------------------------------------------------------------------------------
    void load( const vsettings& settings ) override
    {
        const vsettings *sett_ptr = &settings;

        for ( auto & g: groups )
            sett_ptr = &sett_ptr->subgroup( g.name );

        *ptr = sett_ptr->get<T>( key );
    }
    //-----------------------------------------------------------------------------------
    void save( vsettings* settings ) const override
    {
        for ( auto& g: groups )
            settings = &settings->subgroup( g.name, g.comment );

        settings->set(key, *ptr);
    }
    //-----------------------------------------------------------------------------------
};
//=======================================================================================
template<typename T>
void vsettings::schema::add( cstring key, T *val, cstring comment )
{
    auto ptr = std::make_shared<_node<T>>(key,comment,val);
    _add_node( ptr );
}
//=======================================================================================


#endif // VSETTINGS_H
