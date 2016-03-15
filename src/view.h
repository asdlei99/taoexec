#pragma once

#include <taowin/tw_taowin.h>

#include "core.h"
#include "model.h"
#include "shell.h"
#include "utils.h"

#include <sstream>
#include <functional>
#include <regex>

class INPUTBOX : public taowin::window_creator {
public:
    typedef std::function<bool(INPUTBOX* that, taowin::control* ctl, const std::string& text)> callback_t;
    INPUTBOX(const std::string& text, callback_t cb)
        : _cb(cb)
        , _text(std::move(text)) {
    }

private:
    callback_t _cb;
    const std::string _text;

protected:
    virtual LPCTSTR get_skin_xml() const override {
        return R"tw(
<window title="input" size="500,400">
    <res>
        <font name="default" face="微软雅黑" size="12"/>
        <font name="consolas" face="Consolas" size="12"/>
    </res>
    <root>
        <vertical>
            <edit name="content" font="consolas" style="multiline,wantreturn,vscroll,hscroll" exstyle="clientedge"/>
            <horizontal height="40" padding="5,5,5,5">
                <control/>
                <button name="ok" text="确定" width="40"/>
                <button name="cancel" text="取消" width="40"/>
        </vertical>
    </root>
</window>
)tw";
    }

    virtual LRESULT handle_message(UINT umsg, WPARAM wparam, LPARAM lparam) override {
        if (umsg == WM_CREATE) {
            auto content = _root->find<taowin::edit>("content");
            content->set_text(_text.c_str());
            content->focus();
            return 0;
        }
        return __super::handle_message(umsg, wparam, lparam);
    }

    virtual LRESULT on_notify(HWND hwnd, taowin::control* pc, int code, NMHDR* hdr) override {
        if (pc->name() == "ok" || pc->name() == "cancel") {
            if (code == BN_CLICKED) {
                if (_cb(this, pc, _root->find<taowin::edit>("content")->get_text())) {
                    close();
                    return 0;
                }
            }
        }
        return 0;
    }
};

class ITEM : public taowin::window_creator {
private:
    taoexec::model::item_db_t& _db;
    taoexec::model::item_t*    _item;
    std::function<void(taoexec::model::item_t* p)>          _on_succ;
    std::function<std::string(const std::string& field)>    _on_init;

private:
    taowin::edit*   _id;
    taowin::edit*   _index;
    taowin::edit*   _group;
    taowin::edit*   _comment;
    taowin::edit*   _path;
    taowin::edit*   _params;
    taowin::edit*   _work_dir;
    taowin::edit*   _env;
    taowin::edit*   _show;

public:
    ITEM(taoexec::model::item_db_t& db,
        taoexec::model::item_t* item,
        std::function<void(taoexec::model::item_t*)> on_succ = nullptr,
        std::function<std::string(const std::string& field)> on_init = nullptr)
        : _db(db)
        , _item(item)
        , _on_succ(on_succ)
        , _on_init(on_init) 
    {
    }

protected:
    virtual LPCTSTR get_skin_xml() const override {
        return R"tw(
<window title="item" size="410,280">
    <res>
        <font name="default" face="微软雅黑" size="12" />
    </res>
    <root>
        <vertical padding="5,5,5,5">
            <vertical height="220">
                <horizontal>
                    <label style="centerimage" text="序号" width="70"/>
                    <edit name="id" style="tabstop" exstyle="clientedge" style="disabled"/>
                </horizontal>
                <horizontal>
                    <label style="centerimage" text="索引" width="70"/>
                    <edit name="index" style="tabstop" exstyle="clientedge" />
                </horizontal>
                <horizontal>
                    <label style="centerimage" text="分组" width="70"/>
                    <edit name="group" style="tabstop" exstyle="clientedge" />
                </horizontal>
                <horizontal>
                    <label style="centerimage" text="注释" width="70"/>
                    <edit name="comment" style="tabstop" exstyle="clientedge" />
                </horizontal>
                <horizontal>
                    <label style="centerimage" text="路径" width="70"/>
                    <edit name="path" style="tabstop,readonly" exstyle="clientedge" />
                    <button name="show-path" text="..." width="30"/>
                </horizontal>
                <horizontal>
                    <label style="centerimage" text="参数" width="70"/>
                    <edit name="params" style="tabstop" exstyle="clientedge" />
                </horizontal>
                <horizontal>
                    <label style="centerimage" text="工作目录" width="70"/>
                    <edit name="work_dir" style="tabstop" exstyle="clientedge" />
                </horizontal>
                <horizontal>
                    <label style="centerimage" text="环境变量" width="70"/>
                    <edit name="env" style="tabstop,readonly" exstyle="clientedge" />
                    <button name="show-env" text="..." width="30"/>
                </horizontal>
                <horizontal>
                    <label style="centerimage" text="显示与否" width="70"/>
                    <edit name="show" style="tabstop" exstyle="clientedge" />
                </horizontal>
            </vertical>
            <control minheight="5"/>
            <horizontal height="40">
                <control/>
                <horizontal width="80">
                    <button name="ok" text="保存"/>
                    <button name="cancel" text="取消"/>
                </horizontal> 
            </horizontal>
        </vertical>
    </root>
</window>

)tw";

    }
    virtual LRESULT handle_message(UINT umsg, WPARAM wparam, LPARAM lparam) {
        switch (umsg) {
        case WM_CREATE:
        {
            _id = _root->find<taowin::edit>("id");
            _index = _root->find<taowin::edit>("index");
            _group = _root->find<taowin::edit>("group");
            _comment = _root->find<taowin::edit>("comment");
            _path = _root->find<taowin::edit>("path");
            _params = _root->find<taowin::edit>("params");
            _work_dir = _root->find<taowin::edit>("work_dir");
            _env = _root->find<taowin::edit>("env");
            _show = _root->find<taowin::edit>("show");

            if(_on_init && !_item) {
                _path->set_text(_on_init("path").c_str());
                _params->set_text(_on_init("params").c_str());
                _work_dir->set_text(_on_init("wd").c_str());
                _comment->set_text(_on_init("comment").c_str());
            }

            if (_item) {
                _id->set_text(_item->id.c_str());
                _index->set_text(_item->index.c_str());
                _group->set_text(_item->group.c_str());
                _comment->set_text(_item->comment.c_str());
                _path->set_text(_item->paths.c_str());
                _params->set_text(_item->params.c_str());
                _work_dir->set_text(_item->work_dir.c_str());
                _env->set_text(_item->env.c_str());
                _show->set_text(std::to_string(_item->show).c_str());
            }
            else {
                _id->set_text("-1");
                _show->set_text("1");
            }

            return 0;
        }
        }

        return __super::handle_message(umsg, wparam, lparam);
    }

    virtual bool filter_message(MSG* msg) override {
        if (msg->message == WM_KEYDOWN) {
            switch (msg->wParam) {
            case VK_ESCAPE:
                close();
                return true;
            case VK_RETURN:
            {
                send_message(WM_COMMAND, MAKEWPARAM(BN_CLICKED, 0), LPARAM(_root->find("ok")->hwnd()));
                return true;
            }
            default:
                // I don't want IsDialogMessage to process VK_ESCAPE, because it produces a WM_COMMAND
                // menu message with id == 2. It is undocumented.
                // and, this function call doesn't care the variable _is_dialog.
                if (::IsDialogMessage(_hwnd, msg))
                    return true;
                break;
            }
        }
        return false;
    }

    virtual LRESULT on_notify(HWND hwnd, taowin::control* pc, int code, NMHDR* hdr) {
        if (pc->name() == "ok") {
            if (code == BN_CLICKED) {
                auto p = new taoexec::model::item_t;
                p->id = _id->get_text();
                p->index = _index->get_text();
                p->group = _group->get_text();
                p->comment = _comment->get_text();
                p->paths = _path->get_text();
                p->params = _params->get_text();
                p->work_dir = _work_dir->get_text();
                p->env = _env->get_text();
                p->show = !!std::atoi(_show->get_text().c_str()); // std::stoi will throw

                if (_item == nullptr) {    // create
                    int id;
                    if ((id = _db.insert(p)) <= 0) {    // failed
                        msgbox("失败");
                        delete p;
                        return 0;
                    }
                    else {  // succeeded
                        p->id = std::to_string(id);
                        _id->set_text(p->id.c_str());
                        _item = p;
                        _on_succ(p);
                        close();
                        return 0;
                    }
                }
                else {      // modify
                    if (_db.modify(p)) {
                        _item->index = std::move(p->index);
                        _item->group = std::move(p->group);
                        _item->comment = std::move(p->comment);
                        _item->paths = std::move(p->paths);
                        _item->params = std::move(p->params);
                        _item->work_dir = std::move(p->work_dir);
                        _item->env = std::move(p->env);
                        _item->show = p->show;

                        _on_succ(_item);
                        delete p;
                        close();
                        return 0;
                    }
                    else {
                        msgbox("失败");
                        delete p;
                        return 0;
                    }
                }
                close();
                return 0;
            }
        }
        else if (pc->name() == "cancel") {
            if (code == BN_CLICKED) {
                close();
                return 0;
            }
        }
        else if (pc->name() == "show-env") {
            if (code == BN_CLICKED) {
                INPUTBOX input(_env->get_text(), [&](INPUTBOX* that, taowin::control* ctl, const std::string& text) {
                    if (ctl->name() == "ok")
                        _root->find<taowin::edit>("env")->set_text(text.c_str());
                    return true;
                });
                input.domodal(this);
                return 0;
            }
        }
        else if (pc->name() == "show-path") {
            if (code == BN_CLICKED) {
                INPUTBOX input(_path->get_text(), [&](INPUTBOX* that, taowin::control* ctl, const std::string& text) {
                    if (ctl->name() == "ok")
                        _root->find<taowin::edit>("path")->set_text(text.c_str());
                    return true;
                });
                input.domodal(this);
                return 0;
            }
        }
        return 0;
    }
};

class MINI : public taowin::window_creator {
protected:
    virtual void get_metas(taowin::window::window_meta_t* metas) override {
        __super::get_metas(metas);
        metas->style = WS_POPUP;
        metas->exstyle = WS_EX_TOPMOST | WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    }

    virtual LPCTSTR get_skin_xml() const override {
        return R"tw(
<window title="mini" size="100,18">
    <res>
        <font name="default" face="微软雅黑" size="12" />
        <font name="consolas" face="Consolas" size="12" />
    </res>
    <root>
        <vertical padding="">
            <edit name="args" font="consolas" style="border"/>
        </vertical>
    </root>
</window>
)tw";
    }

    virtual bool filter_message(MSG* msg) override {
        if(msg->message == WM_KEYDOWN) {
            switch(msg->wParam) {
            case VK_ESCAPE:
                close();
                return true;
            case VK_RETURN:
            {
                auto edit = _root->find<taowin::edit>("args");
                execute(edit->get_text());
                return true;
            }
            default:
                // I don't want IsDialogMessage to process VK_ESCAPE, because it produces a WM_COMMAND
                // menu message with id == 2. It is undocumented.
                // and, this function call doesn't care the variable _is_dialog.
                if(::IsDialogMessage(_hwnd, msg))
                    return true;
                break;
            }
        }
        return false;
    }

    virtual LRESULT handle_message(UINT umsg, WPARAM wparam, LPARAM lparam) {
        switch(umsg) {
        case WM_CREATE:
            // move to top of screen, and set attributes
            ([&]() {
                taowin::rect window_rect = get_window_rect();
                int cx_screen = ::GetSystemMetrics(SM_CXSCREEN);
                ::SetWindowPos(_hwnd, nullptr,
                    (cx_screen - window_rect.width()) / 2, 0, 0, 0,
                    SWP_NOSIZE | SWP_NOZORDER);

                ::SetLayeredWindowAttributes(_hwnd, 0, std::atoi(_cfg.get("mini_translucent", "50").c_str()), LWA_ALPHA);
            })();

            ([&]() {
                unsigned int mods, keycode;
                const char* err;

                auto hotkey = _cfg.get("hotkey", "ctrl+shift+z");
                if (taoexec::shell::parse_hotkey_string(hotkey, &mods, &keycode, &err)) {
                    if (!::RegisterHotKey(_hwnd, 0, mods, keycode))
                        msgbox("热键注册失败！", MB_ICONERROR);
                }
                else {
                    msgbox("无效热键组合：" + std::string(err), MB_ICONEXCLAMATION);
                }
            })();

            init_commanders();

            _root->find("args")->focus();
            return 0;
        case WM_HOTKEY:
        {
            if(wparam == 0) {
                set_display(2);
            }
            return 0;
        }
        case WM_SETFOCUS:
            if(auto edit = _root->find("args"))
                edit->focus();
            break;
        }
        return __super::handle_message(umsg, wparam, lparam);
    }

    void set_display(int cmd) {
        if(cmd == 2)
            cmd = ::IsWindowVisible(_hwnd) ? 0 : 1;

        if(cmd == 1) {
            if(!::IsWindowVisible(_hwnd))
                ::ShowWindow(_hwnd, SW_SHOW);
            ::SetForegroundWindow(_hwnd);
            ::SetActiveWindow(_hwnd);
            _root->find("args")->focus();
        }
        else {
            ::ShowWindow(_hwnd, SW_HIDE);
            _root->find<taowin::edit>("args")->set_text("");
        }
    }

    virtual void on_final_message() override {
        delete this;
    }

public:
    MINI(taoexec::model::item_db_t& db, taoexec::model::config_db_t& cfg)
        : _db(db)
        , _cfg(cfg)
        , _focus(nullptr) {}

    ~MINI() {
        for (auto& it : _commanders)
            delete it.second;
    }

private:
    HWND _focus;
    taoexec::model::item_db_t& _db;
    taoexec::model::config_db_t& _cfg;

private:
    class command_executor_i;
    std::map <std::string, command_executor_i*> _commanders;

    class command_executor_i {
    public:
        virtual const std::string get_name() const = 0;
        virtual bool execute(const std::string& args) = 0;
    };

    class executor_main : public command_executor_i {
    private:
        MINI* _pmini;
        std::map<std::string, std::function<void()>> _cmds;

    public:
        executor_main(MINI* pmini)
            : _pmini(pmini)
        {
            _cmds[""] = [&]() {
                _pmini->set_display(0);
            };

            _cmds["main"] = [&]() {
                void create_main(taoexec::model::item_db_t& db, taoexec::model::config_db_t& cfg);
                create_main(_pmini->_db, _pmini->_cfg);
            };

            _cmds["exit"] = [&]() {
                _pmini->close();
            };
        }

        const std::string get_name() const override{
            return "__main__";
        }

        bool execute(const std::string& args) override {
            auto found = _cmds.find(args);
            if (found != _cmds.end()) {
                found->second();
                return true;
            }
            else {
                _pmini->msgbox("无此内建命令。");
                return false;
            }
        }
    };

    class executor_indexer : public command_executor_i {
    private:
        MINI* _pmini;
        taoexec::model::item_db_t* _itemdb;

    public:
        executor_indexer(MINI* pmini, taoexec::model::item_db_t* itemdb)
            : _pmini(pmini)
            , _itemdb(itemdb) 
        {
        }

        const std::string get_name() const override {
            return "__indexer__";
        }

        bool execute(const std::string& args) override {
            std::string index, params;

            auto p = args.c_str();
            auto bp = p;

            while (*p && ::isalnum(*p))
                ++p;

            if (*p == '\0') { // 没有参数
                index = bp;
            }
            else {
                index = std::string(bp, p - bp);

                while (*p == ' ' || *p == '\t')
                    ++p;
                bp = p;
                for (p = args.c_str() + args.size() - 1; *p == ' ' || *p == '\t';)
                    --p;
                params = std::string(bp, p + 1);
            }

            auto from_db = [&]()->taoexec::model::item_t* {
                taoexec::model::item_t* found = nullptr;
                std::vector<taoexec::model::item_t*> items;
                int rc = _itemdb->query(index, &items);
                if (rc == -1) {
                    _pmini->msgbox("sqlite3 error.");
                }
                else if (rc == 0) {
                    _pmini->msgbox("Your search `" + index + "` does not match anything.");
                }
                else if (rc == 1) {
                    found = items[0];
                }
                else {
                    decltype(items.cbegin()) it;
                    for (it = items.cbegin(); it != items.cend(); it++) {
                        if ((*it)->index == index) {
                            found = *it;
                            break;
                        }
                    }

                    for (auto pi : items) {
                        if (pi != found)
                            delete pi;
                    }

                    if (found == nullptr) {
                        _pmini->msgbox("There are many rows that match your given prefix.");
                    }
                    // found
                }

                return found;
            };

            auto item = from_db();

            if (!item) {
                return false;
            }

            std::vector<std::string> patharr;
            taoexec::utils::split_paths(item->paths, &patharr);
            taoexec::core::execute(_pmini->_hwnd, patharr, item->params, params, item->work_dir, item->env, nullptr);

            delete item;

            return true;
        }
    };

    class executor_qq : public command_executor_i {
    private:
        taoexec::model::config_db_t& _cfg;
        std::string _uin;
        std::string _path;

        struct _nickname_compare {
            bool operator()(const std::string& lhs, const std::string& rhs) {
                return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
            }
        };

        std::map<std::string, std::string, _nickname_compare> _users;

    public:
        executor_qq(taoexec::model::config_db_t& cfg)
            : _cfg(cfg) 
            , _uin("191035066")
        {
            _path = _cfg.get("qq_path");

            auto userstr = _cfg.get("qq_users");
            std::istringstream iss(userstr);

            for (std::string line; std::getline(iss, line, '\n');) {
                std::regex user_n_hash(R"(([0-9a-zA-Z]+),([0-9A-F]{80})\r*)");
                std::smatch tokens;

                if (std::regex_match(line, tokens, user_n_hash)) {
                    std::string nickname = tokens[1].str();
                    std::string hash = tokens[2].str();

                    _users[nickname] = hash;
                }
            }
        }

        const std::string get_name() const override {
            return "qq";
        }

        bool execute(const std::string& args) override {
            auto it = _users.find(args);
            if (it != _users.end()) {
                std::string cmd = std::string("/uin:") + _uin + " /quicklunch:" + it->second;
                ::ShellExecute(::GetActiveWindow(), "open", _path.c_str(), cmd.c_str(), nullptr, SW_SHOWNORMAL);
                return true;
            }
            return false;
        }
    };

    /* 初始化命令执行者
     *  带双下划线的是预定义的执行者
    */
    void init_commanders() {
        command_executor_i* pexec = nullptr;

        pexec = new executor_main(this);
        _commanders[pexec->get_name()] = pexec;

        pexec = new executor_indexer(this, &this->_db);
        _commanders[pexec->get_name()] = pexec;

        pexec = new executor_qq(_cfg);
        _commanders[pexec->get_name()] = pexec;
    }

protected:

    virtual LRESULT on_notify(HWND hwnd, taowin::control* pc, int code, NMHDR* hdr) {
        return 0;
    }

    // 命令行支持的模式：
    /*
     *  [commander] [:] [command-specs]
     **/
    void execute(std::string& __args) {
        auto execute_commander = [&](const std::string& commander, const std::string& args) {
            auto it = _commanders.find(commander);
            if(it != _commanders.end()) {
                if (it->second->execute(args))
                    set_display(0);
                return;
            }
            else {
                msgbox("未找到执行者。");
                return;
            }
        };

        std::string commander, args;

        auto p = __args.c_str();
        char c;

        for(;;) {
            c = *p;
            if(c == ' ' || c == '\t') {
                ++p;
                continue;
            }
            else if(c == '\0') {
                commander = "__main__";
                execute_commander(commander, "");
                goto _break;
            }
            // 到了这里并不知道有没有提供执行者，找到冒号才能确定
            else if(::isalnum(c) || c == ':') {
                auto bp = p; // p's backup
                while((c=*p) && c != ':' && ::isalnum(c))
                    ++p;

                if(c == ':') {  // 找到了执行者
                    commander = bp == p ? "__main__" : std::string(bp, p - bp);
                    args = ++p;
                    execute_commander(commander, args);
                    goto _break;
                }
                else { // 其它则全部判断为 __indexer__
                    commander = "__indexer__";
                    args = std::string(bp);
                    execute_commander(commander, args);
                    goto _break;
                }

            }
            else {
                msgbox("无法识别的命令行。");
                goto _break;
            }
        }

    _break:
        return;
    }
};

class CONFIG : public taowin::window_creator {
private:
    using config_db_t = taoexec::model::config_db_t;

    config_db_t&                        _cfg;
    std::vector<config_db_t::item_t*>   _items;

    HMENU                               _hmenu;

    enum class menuid {
        none,
        add,
        modify,
        remove,
    };

    class INPUT : public taowin::window_creator {
    public:
        using callback_t = std::function<bool(const std::string& name, const std::string& value, const std::string& comment)>;

    protected:
        const config_db_t::item_t* _item;


        callback_t _onok;
        //callback_t _oncancel;

    protected:
        virtual void get_metas(taowin::window::window_meta_t* metas) override {
            __super::get_metas(metas);
        }

        virtual LPCTSTR get_skin_xml() const override {
            return R"tw(
<window title="INPUT" size="370,300">
    <res>
        <font name="default" face="微软雅黑" size="12" />
        <font name="consolas" face="Consolas" size="12" />
    </res>
    <root>
        <vertical padding="10,10,10,10">
            <label text="Name: " height="20" />
            <edit name="name" font="consolas" style="" exstyle="clientedge" height="20"/>
            <control height="10" />
            <label text="Comment: " height="20" />
            <edit name="comment" font="consolas" style="" exstyle="clientedge" height="20"/>
            <control height="10" />
            <label text="Value: " height="20"/>
            <edit name="value" font="consolas" style="multiline,wantreturn" exstyle="clientedge" minheight="100"/>
            <control height="15"/>
            <horizontal height="30">
                <control />
                <button name="ok" width="40" text="确定"/>
                <control width="10"/>
                <button name="cancel" width="40" text="取消"/>
            </horizontal>
        </vertical>
    </root>
</window>
)tw";
        }

        virtual bool filter_message(MSG* msg) override {
            if (msg->message == WM_KEYDOWN) {
                switch (msg->wParam) {
                case VK_ESCAPE:
                    close();
                    return true;
                default:
                    // I don't want IsDialogMessage to process VK_ESCAPE, because it produces a WM_COMMAND
                    // menu message with id == 2. It is undocumented.
                    // and, this function call doesn't care the variable _is_dialog.
                    if (::IsDialogMessage(_hwnd, msg))
                        return true;
                    break;
                }
            }
            return false;
        }

        virtual LRESULT handle_message(UINT umsg, WPARAM wparam, LPARAM lparam) {
            switch (umsg) {
            case WM_CREATE:
            {
                auto name = _root->find<taowin::edit>("name");
                auto value = _root->find<taowin::edit>("value");
                auto comment = _root->find<taowin::edit>("comment");

                if (_item) {
                    name->set_text(_item->name.c_str());
                    value->set_text(_item->value.c_str());
                    comment->set_text(_item->comment.c_str());

                    name->set_enabled(false);
                    value->focus();
                }
                else {
                    name->focus();
                }

                return 0;
            }
            }
            return __super::handle_message(umsg, wparam, lparam);
        }

    public:
        INPUT(const config_db_t::item_t* item = nullptr, callback_t onok = nullptr)
            : _item(item)
            , _onok(onok)
            //, _oncancel(nullptr)
        {
        }

    protected:
        virtual LRESULT on_notify(HWND hwnd, taowin::control* pc, int code, NMHDR* hdr) {
            if (pc->name() == "cancel") {
                close();
                return 0;
            }
            else if (pc->name() == "ok") {
                auto name = _root->find<taowin::edit>("name");
                auto value = _root->find<taowin::edit>("value");
                auto comment = _root->find<taowin::edit>("comment");
                if (_onok(name->get_text(), value->get_text(), comment->get_text())) {
                    close();
                    return 0;
                }
                return 0;
            }
            return 0;
        }
    };

public:
    CONFIG(taoexec::model::config_db_t& cfg)
        : _cfg(cfg)
        , _hmenu(nullptr)
    {
    }

protected:
    virtual LPCTSTR get_skin_xml() const override {
        return R"tw(
<window title="CONFIG" size="600,400">
    <res>
        <font name="default" face="微软雅黑" size="12" />
    </res>
    <root>
        <vertical padding="5,5,5,5">
            <listview name="lv" style="showselalways,ownerdata,singlesel" exstyle="clientedge" minwidth="550"/>
        </vertical>
    </root>
</window>

)tw";
    }

    menuid show_menu() {
        auto lv = _root->find<taowin::listview>("lv");
        int nsel = lv->get_selected_count();

        ::EnableMenuItem(_hmenu, (UINT)menuid::modify, MF_BYCOMMAND | (nsel == 1 ? MF_ENABLED : MF_DISABLED | MF_GRAYED));
        ::EnableMenuItem(_hmenu, (UINT)menuid::remove, MF_BYCOMMAND | (nsel == 1 ? MF_ENABLED : MF_DISABLED | MF_GRAYED));

        ::POINT pt;
        ::GetCursorPos(&pt);
        return (menuid)::TrackPopupMenuEx(_hmenu, TPM_LEFTBUTTON | TPM_RETURNCMD, pt.x, pt.y, _hwnd, nullptr);
    }

    virtual LRESULT handle_message(UINT umsg, WPARAM wparam, LPARAM lparam) {
        switch(umsg)
        {
        case WM_CREATE: {
            ([&]() {
                static const struct {
                    const char* name;
                    int width;
                } cols[] = {
                    { "name", 100 },
                    { "value", 200 },
                    { "comment", 200 },
                    { nullptr, 0 }
                };

                auto lv = _root->find<taowin::listview>("lv");
                for (int i = 0; i < _countof(cols); ++i) {
                    auto col = &cols[i];
                    lv->insert_column(col->name, col->width, i);
                }

                _cfg.query("", &_items);

                lv->set_item_count(_items.size(), 0);
            })();

            ([&]() {
                _hmenu = CreatePopupMenu();
                ::AppendMenu(_hmenu, MF_STRING, (UINT_PTR)menuid::add,       "增加");
                ::AppendMenu(_hmenu, MF_STRING, (UINT_PTR)menuid::modify,    "修改");
                ::AppendMenu(_hmenu, MF_STRING, (UINT_PTR)menuid::remove ,   "删除");
            })();

            return 0;
        }
        }

        return __super::handle_message(umsg, wparam, lparam);
    }

    virtual bool filter_message(MSG* msg) override {
        if(msg->message == WM_KEYDOWN) {
            switch(msg->wParam) {
            case VK_ESCAPE:
                close();
                return true;
            case VK_RETURN:
            {
                send_message(WM_COMMAND, MAKEWPARAM(BN_CLICKED, 0), LPARAM(_root->find("ok")->hwnd()));
                return true;
            }
            default:
                // I don't want IsDialogMessage to process VK_ESCAPE, because it produces a WM_COMMAND
                // menu message with id == 2. It is undocumented.
                // and, this function call doesn't care the variable _is_dialog.
                if(::IsDialogMessage(_hwnd, msg))
                    return true;
                break;
            }
        }
        return false;
    }

    virtual LRESULT on_notify(HWND hwnd, taowin::control* pc, int code, NMHDR* hdr) {
        if (pc->name() == "lv") {
            auto lv = reinterpret_cast<taowin::listview*>(pc);
            if (code == NM_RCLICK) {
                switch (show_menu()) {
                case menuid::add:
                {
                    INPUT input(nullptr, [&](const std::string& name, const std::string& value, const std::string& comment) {
                        _cfg.set(name, value, comment);
                        auto item = new config_db_t::item_t;
                        item->name = name;
                        item->value = value;
                        item->comment = comment;
                        _items.push_back(item);
                        lv->set_item_count(_items.size(), 0);
                        return true;
                    });
                    input.domodal(this);
                    break;
                }
                case menuid::modify:
                {
                    auto nmlv = reinterpret_cast<NMLISTVIEW*>(hdr);
                    auto it = _items.begin() + nmlv->iItem;
                    INPUT input(*it, [&](const std::string& name, const std::string& value, const std::string& comment) {
                        (*it)->value = value;
                        (*it)->comment = comment;
                        lv->redraw_items(nmlv->iItem, nmlv->iItem);
                        _cfg.set(name, value, comment);
                        return true;
                    });
                    input.domodal(this);
                    break;
                }
                case menuid::remove:
                {
                    auto nmlv = reinterpret_cast<NMLISTVIEW*>(hdr);
                    auto it = _items.begin() + nmlv->iItem;

                    _cfg.set((*it)->name, "", "");
                    lv->delete_item(nmlv->iItem);
                    _items.erase(it);

                    break;
                }
                case menuid::none:
                default:
                    break;
                }

                return 0;
            }
            else if (code == NM_DBLCLK) {
                auto nmlv = reinterpret_cast<NMITEMACTIVATE*>(hdr);
                if (nmlv->iItem != -1) {
                    auto it = _items.begin() + nmlv->iItem;
                    INPUT input(*it, [&](const std::string& name, const std::string& value, const std::string& comment) {
                        (*it)->value = value;
                        (*it)->comment = comment;
                        lv->redraw_items(nmlv->iItem, nmlv->iItem);
                        _cfg.set(name, value, comment);
                        return true;
                    });
                    input.domodal(this);
                    return 0;
                }
            }
            else if (code == LVN_GETDISPINFO) {
                NMLVDISPINFO* pdi = reinterpret_cast<NMLVDISPINFO*>(hdr);
                auto rit = _items[pdi->item.iItem];
                auto lit = &pdi->item;
                switch (lit->iSubItem) {
                case 0: lit->pszText = (LPSTR)rit->name.c_str(); break;
                case 1:lit->pszText = (LPSTR)rit->value.c_str(); break;
                case 2:lit->pszText = (LPSTR)rit->comment.c_str(); break;
                }
                return 0;
            }
        }
        return __super::on_notify(hwnd, pc, code, hdr);
    }
};

class TW : public taowin::window_creator {
private:
    taoexec::model::item_db_t& _db;
    taoexec::model::config_db_t& _cfg;
    std::vector<taoexec::model::item_t*> _items;

public:
    TW(taoexec::model::item_db_t& db, taoexec::model::config_db_t& cfg) 
        : _db(db)
        , _cfg(cfg)
    {
    }

protected:
    virtual void get_metas(window_meta_t* metas) {
        __super::get_metas(metas);
        metas->exstyle |= WS_EX_ACCEPTFILES;
    }

    virtual LPCTSTR get_skin_xml() const override {
        return R"tw(
<window title="taoexec" size="850,600">
    <res>
        <font name="default" face="微软雅黑" size="12"/>
    </res>
    <root>
        <horizontal padding="5,5,5,5" minheight="150">
            <listview name="list" style="showselalways,ownerdata" exstyle="clientedge" minwidth="300"/>
            <vertical width="80" padding="5,0,0,0">
                <vertical height="100">
                    <button name="refresh" text="刷新"/>
                    <button name="add" text="添加"/>
                    <button name="modify" text="修改" style="disabled"/>
                    <button name="delete" text="删除" style="disabled"/>
                </vertical>
                <control/>
                <button name="settings" text="设置中心" height="25"/>
                <button name="mini" text="小窗" height="25"/>
            </vertical>
        </horizontal>
    </root>
</window>
)tw";
    }

    virtual LRESULT handle_message(UINT umsg, WPARAM wparam, LPARAM lparam) override {
        switch(umsg) {
        case WM_CREATE:
        {
            center();
            auto lv = _root->find<taowin::listview>("list");
            assert(lv);

            struct {
                const char* name;
                int         width;
            } cols[] = {
                {"id",50},
                {"index", 80},
                {"group", 80},
                {"comment", 100},
                {"path", 100},
                {"params", 100},
                {"work_dir", 80},
                {"env", 100},
                {"show", 50},
                {nullptr, 0}
            };

            for(int i = 0; i < _countof(cols); i++) {
                auto col = &cols[i];
                lv->insert_column(col->name, col->width, i);
            }

            // 设置管理员标题
            ([](HWND hwnd) {
                char tt[1024];
                tt[::GetWindowText(hwnd, tt, _countof(tt)-128/* enough */)] = '\0';
                ::strcat(tt, ::IsUserAnAdmin() ? " (Admin)" : " (non-Admin)");
                ::SetWindowText(hwnd, tt);
            })(_hwnd);

            _refresh();

            return 0;
        }
        case WM_DROPFILES:
        {
             taoexec::shell::drop_files(HDROP(wparam)).for_each([&](int i, const std::string& path) {
                 using namespace taoexec::shell;

                 // TODO 复用
                 taowin::listview* lv = _root->find<taowin::listview>("list");
                 // callback
                 auto on_added = [&](taoexec::model::item_t* p) {
                     if (_items.size() == 0 || _items.back() != p)
                         _items.push_back(p);
                     int count = _items.size();
                     lv->set_item_count(count, LVSICF_NOINVALIDATEALL);
                     lv->redraw_items(count - 1, count - 1);
                 };

                 if(type(path.c_str()) == file_type::file) {
                     auto ext = taoexec::shell::ext(path);
                     if(is_ext_link(ext)) {
                         link_info info;
                         if(parse_link_file(path, &info)) {
                             // 不打算再解析Shell Item IDList信息了，看了下面的文章简直吓尿
                             // https://github.com/libyal/libfwsi/blob/master/documentation/Windows%20Shell%20Item%20format.asciidoc
                             // http://forensicswiki.org/wiki/LNK
                             if (!info.path.size()) {
                                 msgbox(path + "\n\n" + "不支持的快捷方式文件。", MB_ICONERROR);
                                 return;
                             }

                             auto on_init = [&](const std::string& field) {
                                 if(field == "path")
                                     return info.path;
                                 else if(field == "params")
                                     return info.args;
                                 else if(field == "wd")
                                     return info.wd;
                                 else if(field == "comment")
                                     return info.desc;
                                 else
                                     return std::string();
                             };

                             ITEM item(_db, nullptr, on_added, on_init);
                             item.domodal(this);
                         }
                     }
                     /*
                     // EXE 可以获取到一些特殊的资源块信息，所以可以单独写一个添加函数
                     else if (stricmp(ext.c_str(), ".exe") == 0) {

                     }
                     */
                     else {
                         auto on_init = [&](const std::string& field)->std::string {
                             if (field == "path")
                                 return path;

                             return std::string();
                         };

                         ITEM item(_db, nullptr, on_added, on_init);
                         item.domodal(this);
                     }
                 }
             });

             return 0;
        }
        default:
            break;
        }
        return __super::handle_message(umsg, wparam, lparam);
    }

    virtual LRESULT on_notify(HWND hwnd, taowin::control* pc, int code, NMHDR* hdr) override {
        if(pc->name() == "list") {
            if(!hdr) return 0;
            taowin::listview* list = static_cast<taowin::listview*>(pc);
            if(code == LVN_GETDISPINFO) {
                NMLVDISPINFO* pdi = reinterpret_cast<NMLVDISPINFO*>(hdr);
                auto rit = _items[pdi->item.iItem]; // right-hand item
                auto lit = &pdi->item;              // left-hand item
                switch(lit->iSubItem) 
                {
                case 0: lit->pszText = (LPSTR)rit->id.c_str(); break;
                case 1: lit->pszText = (LPSTR)rit->index.c_str(); break;
                case 2: lit->pszText = (LPSTR)rit->group.c_str(); break;
                case 3: lit->pszText = (LPSTR)rit->comment.c_str(); break;
                case 4: lit->pszText = (LPSTR)rit->paths.c_str(); break;
                case 5: lit->pszText = (LPSTR)rit->params.c_str(); break;
                case 6: lit->pszText = (LPSTR)rit->work_dir.c_str(); break;
                case 7: lit->pszText = (LPSTR)rit->env.c_str(); break;
                case 8: lit->pszText = (LPSTR)(rit->show ? "1" : "0"); break;
                }
            }
            else if(code == LVN_ITEMCHANGED
                || code == LVN_DELETEITEM
                || code == LVN_DELETEALLITEMS
                )
            {
                auto btn_modify = _root->find<taowin::button>("modify");
                auto btn_delete = _root->find<taowin::button>("delete");
                btn_modify->set_enabled(list->get_selected_count() == 1);
                btn_delete->set_enabled(list->get_selected_count() > 0);

                if(code == LVN_DELETEITEM) {
                    auto lv = reinterpret_cast<NMLISTVIEW*>(hdr);
                    auto it = _items.begin() + lv->iItem;   // TODO make post
                    _items.erase(it);
                } else if(code == LVN_DELETEALLITEMS) {
                    // never happens.
                }

                return 0;
            }
            else if(code == NM_DBLCLK) {
                auto item = reinterpret_cast<NMITEMACTIVATE*>(hdr);
                _execute(item->iItem);
                return 0;
            }
        }
        else if(pc->name() == "add" || pc->name() == "modify") {
            if(code != BN_CLICKED) 
                return 0;
            if(pc->name() == "add") {
                taowin::listview* lv = _root->find<taowin::listview>("list");
                // callback
                auto on_added = [&](taoexec::model::item_t* p) {
                    if(_items.size()==0 || _items.back() != p)
                        _items.push_back(p);
                    int count = _items.size();
                    lv->set_item_count(count, LVSICF_NOINVALIDATEALL);
                    lv->redraw_items(count - 1, count - 1);
                };

                ITEM item(_db, nullptr, on_added);
                item.domodal(this);
                return 0;
            }
            else if(pc->name() == "modify") {
                taowin::listview* lv = _root->find<taowin::listview>("list");
                int lvid = lv->get_next_item(-1, LVNI_SELECTED);
                if(lvid == -1) return 0;

                taoexec::model::item_t& it = *_items[lvid];

                // callback
                auto on_modified = [&](taoexec::model::item_t* p) {
                    lv->redraw_items(lvid, lvid);
                };

                ITEM item(_db, &it, on_modified);
                item.domodal(this);

                return 0;
            }
        }
        else if(pc->name() == "delete") {
            if(code != BN_CLICKED)
                return 0;

            auto list = static_cast<taowin::listview*>(_root->find("list"));
            int count = list->get_selected_count();
            if(msgbox(("确定要删除选中的 " + std::to_string(count) + "项？").c_str(), MB_OKCANCEL) == IDOK) {
                int index = -1;
                std::vector<int> selected;
                while((index = list->get_next_item(index, LVNI_SELECTED)) != -1) {
                    selected.push_back(index);
                }

                for(auto it = selected.crbegin(); it != selected.crend(); it++) {
                    int id = std::atoi(_items[*it]->id.c_str());
                    if(_db.remove(id)) {
                        if(list->delete_item(*it)) {
                            continue;
                        }
                    }
                    msgbox("删除失败！");
                    break;
                }
            }
            return 0;
        }
        else if(pc->name() == "refresh") {
            _refresh();
            return 0;
        }
        else if(pc->name() == "mini") {
            MINI* mini = new MINI(_db, _cfg);
            mini->create();
            mini->show();
            return 0;
        }
        else if(pc->name() == "settings") {
            CONFIG cfg(_cfg);
            cfg.domodal(this);
            return 0;
        }
        return 0;
    }

    virtual void on_final_message() override {
        delete this;
    }

private:
    void _refresh() {
        _db.query("", &_items);

        taowin::listview* lv = _root->find<taowin::listview>("list");
        lv->set_item_count(_items.size(), 0);   // cause invalidate all.
    }

    void _execute(int i) {
        if(i<0 || i>(int)_items.size() - 1)
            return;

        auto& paths      = _items[i]->paths;
        auto& params    = _items[i]->params;
        auto& wd        = _items[i]->work_dir;
        auto& env       = _items[i]->env;

        std::vector<std::string> patharr;
        taoexec::utils::split_paths(paths, &patharr);

        taoexec::core::execute(_hwnd, patharr, params, "", wd, env, [&](const std::string& err) {
            if(err != "ok") {
                msgbox("失败。", MB_ICONEXCLAMATION);
            }
        });

    }
};

void create_main(taoexec::model::item_db_t& db, taoexec::model::config_db_t& cfg) {
    TW* ptw = new TW(db, cfg);
    ptw->create();
    ptw->show();
}
