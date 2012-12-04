#ifndef FUNGUSUTIL_CLONE_MAP_H
#define FUNGUSUTIL_CLONE_MAP_H

#include <list>

namespace fungus_util
{
    enum clone_map_mode_e
    {
        MAP_GLOBAL,
        MAP_REPLICATE
    };

    template <typename T>
    class pred_true
    {
    public:
        inline bool operator()(const T &a, const T &b) const {return true;}
    };

    template <typename T>
    class clonable_copy
    {
    public:
        inline T *operator()(const T *o)
        {
            return new T();
        }
    };

    template <typename T>
    class clonable_get
    {
    public:
        inline typename T::clonable_type *operator()(T *o)
        {
            return o;
        }
    };

    template <clone_map_mode_e mapMode, typename T,
              typename dataT, typename getT>
    class __clone_map;

    template <typename T,
              typename dataT, typename getT>
    class __clone_map<MAP_GLOBAL, T, dataT, getT>
    {
    private:
        getT get;
        dataT data;
        size_t refs;
    public:
        __clone_map(const dataT &data, getT _get): get(_get), data(data), refs(0) {}
        ~__clone_map() {fungus_util_assert(refs == 0, "__clone_map deleted with refs != 0!");}

        inline void add(T *obj)
        {
            if (get(obj)->_map == this) return;

            if (get(obj)->_map)
            {
                __clone_map *mmap = get(obj)->_map;
                mmap->remove(obj);
                if (mmap->empty()) delete mmap;
            }

            get(obj)->_map = this;
            ++refs;
        }

        inline void remove(T *obj)
        {
            if (get(obj)->_map != this) return;
            get(obj)->_map = nullptr;
            --refs;
        }

        inline void global_data(const dataT &data)
        {
            this->data = data;
        }

        inline const dataT &global_data() const
        {
            return data;
        }

        inline bool empty() const
        {
            return refs == 0;
        }
    };

    template <typename T,
              typename dataT, typename getT>
    class __clone_map<MAP_REPLICATE, T, dataT, getT>
    {
    private:
        typedef typename std::list<T *> obj_list;
        typedef typename obj_list::iterator obj_list_it;
        typedef typename obj_list::const_iterator obj_list_const_it;

        obj_list objs;
        getT get;
        dataT data;
    public:
        __clone_map(const dataT &data, getT _get): objs(), get(_get), data(data) {objs.clear();}
        ~__clone_map() {clear();}

        inline void clear()
        {
            for (obj_list_it it = objs.begin(); it != objs.end(); ++it)
            {
                T *obj = *it;
                get(obj)->_map = nullptr;
            }

            objs.clear();
        }

        inline void add(T *obj)
        {
            if (get(obj)->_map == this) return;

            objs.push_back(obj);
            if (get(obj)->_map)
            {
                __clone_map *mmap = get(obj)->_map;
                mmap->remove(obj);
                if (mmap->empty()) delete mmap;
            }

            get(obj)->_map = this;
        }

        inline void remove(T *obj)
        {
            if (get(obj)->_map != this) return;

            objs.remove(obj);
            get(obj)->_map = nullptr;
        }

        template <typename predT>
        inline void replicate_data(const dataT &data, predT pred)
        {
            for (obj_list_it it = objs.begin(); it != objs.end(); ++it)
            {
                T *obj = *it;
                get(obj)->_data = pred(get(obj)->_data, data) ? data : get(obj)->_data;
            }
        }

        inline void global_data(const dataT &data)
        {
            this->data = data;
        }

        inline const dataT &global_data() const
        {
            return data;
        }

        inline bool empty() const
        {
            return objs.empty();
        }
    };

    template <clone_map_mode_e mapMode, typename T,
              typename dataT = any_type,
              typename copyT = clonable_copy<T>,
              typename getT  = clonable_get<T> >
    class clonable
    {
    private:
        typedef __clone_map<mapMode, T, dataT, getT> __map_impl;
        friend class __clone_map<mapMode, T, dataT, getT>;

        T *_self;
        __map_impl *_map;
        dataT _data, _tmp_glob_data;

        copyT _copy;
        getT _get;
    public:
        typedef clonable<mapMode, T, dataT, copyT, getT> clonable_type;

        clonable(T *_self, const dataT &data = dataT(), copyT __copy = clonable_copy<T>(), getT __get = clonable_get<T>()):
            _self(_self), _map(nullptr), _data(data), _tmp_glob_data(data), _copy(__copy), _get(__get) {}

        ~clonable()
        {
            if (_map)
            {
                __map_impl *mmap = _map;
                mmap->remove(_self);
                if (mmap->empty()) delete mmap;
            }
        }

        inline T *copy()
        {
            return _copy(_self);
        }

        inline T *clone()
        {
            T *nobj = copy();

            if (!_map)
            {
                __map_impl *mmap = new __map_impl(_tmp_glob_data, _get);
                mmap->add(_self);
            }

            _map->add(nobj);
            return nobj;
        }

        inline void data(const dataT &data) {_data = data;}
        inline const dataT &data() const    {return _data;}

        inline       T *self()              {return _self;}
        inline const T *self() const        {return _self;}

        template <typename predT>
        inline void replicate_data(predT pred)
        {
            if (_map) _map->replicate_data(_data, pred);
        }

        inline void replicate_data()
        {
            if (_map) _map->replicate_data(_data, pred_true<dataT>());
        }

        inline void global_data(const dataT &data)
        {
            if (_map)
                _map->global_data(data);
            else
                _tmp_glob_data = data;
        }

        inline const dataT &global_data() const
        {
            if (_map)
                return _map->global_data();
            else
                return _tmp_glob_data;
        }
    };
};

#endif
