from sqlalchemy import text
from connect import engine

# 회사별 가장 최근 작성된 블로그 조회
def get_latest_blog_by_company():
    # 쿼리문 작성
    query = text("""
        SELECT C.COMPANY_NAME, B.TITLE, B.PUBLISHED_DATE AS 최신게시물_업로드일자
        FROM COMPANY C
        JOIN BLOG B ON C.COMPANY_ID = B.COMPANY_ID
        WHERE B.PUBLISHED_DATE = (
            SELECT MAX(B2.PUBLISHED_DATE)
            FROM BLOG B2
            WHERE B2.COMPANY_ID = C.COMPANY_ID
        );
    """)
    # DB 연결 및 쿼리문 실행
    with engine.connect() as conn:
        result = conn.execute(query)
        
        # 결과(result) 딕셔너리로 변환
        keys = result.keys()
        return [dict(zip(keys, row)) for row in result]

# 특정 태그가 포함된 블로그 글 조회
def get_blogs_with_tag(tag_type):
    # 쿼리문 작성
    query = text("""
        SELECT TITLE
        FROM BLOG
        WHERE BLOG_ID IN (
            SELECT BT.BLOG_ID
            FROM BLOG_TAG BT
            WHERE BT.TAG_ID IN (
                SELECT TAG_ID
                FROM TAG
                WHERE TAG_TYPE = :tag_type
            )
        );
    """)
    # DB 연결 및 쿼리문 실행
    with engine.connect() as conn:
        result = conn.execute(query, {"tag_type": tag_type})
        
        # 결과(result) 딕셔너리로 변환
        keys = result.keys()
        return [dict(zip(keys, row)) for row in result]

# 번역된 블로그의 회사별 개수 조회
def get_translated_blog_count_by_company():
    # 쿼리문 작성
    query = text("""
        SELECT C.COMPANY_NAME, COUNT(T.BLOG_ID) AS 번역된_블로그개수
        FROM COMPANY C
        LEFT JOIN BLOG B ON C.COMPANY_ID = B.COMPANY_ID
        LEFT JOIN TRANSLATION T ON B.BLOG_ID = T.BLOG_ID
        GROUP BY C.COMPANY_NAME;
    """)

    # DB 연결 및 쿼리문 실행
    with engine.connect() as conn:
        result = conn.execute(query)
        
        # 결과(result) 딕셔너리로 변환
        keys = result.keys()
        return [dict(zip(keys, row)) for row in result]

# 테스트 실행
if __name__ == "__main__":
    print("회사별 가장 최근 작성된 블로그 조회:")
    print(get_latest_blog_by_company())

    print("\n'AI' 태그가 포함된 블로그 조회:")
    print(get_blogs_with_tag("AI"))

    print("\n번역된 블로그의 회사별 개수 조회:")
    print(get_translated_blog_count_by_company())