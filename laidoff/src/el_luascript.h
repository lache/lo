#pragma once
#include "litehtml.h"

typedef struct _LWCONTEXT LWCONTEXT;

namespace litehtml
{
	class el_luascript : public html_tag
	{
	public:
        el_luascript(const std::shared_ptr<litehtml::document>& doc, LWCONTEXT* pLwc);
		virtual ~el_luascript();

	protected:
		virtual void	parse_attributes() override;
        //virtual bool			appendChild(const element::ptr &el) override;
        virtual const tchar_t*	get_tagName() const override;
    private:
        LWCONTEXT * pLwc;
	};
}
